// SessionGroup.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SessionGroup.h"
#include "ppbox/dispatch/Session.h"
#include "ppbox/dispatch/TaskDispatcher.h"
#include "ppbox/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

namespace ppbox
{
    namespace dispatch
    {

        FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.SessionGroup", framework::logger::Debug);

        static char const * const status_str[] = {
            "waiting", 
            "openning", 
            "openned", 
            "working", 
            "buffering", 
        };

#define LOG_XXX(p) LOG_DEBUG("[" p "] status:" << status_str[status_] << " current:" << current_ << " next:" << next_);

        Request * SessionGroup::open_request = (Request *)0x01;
        Request * SessionGroup::switch_request = (Request *)0x02;
        Request * SessionGroup::buffer_request = (Request *)0x03;
        Request * SessionGroup::cancel_request = (Request *)0x04;
        Request * SessionGroup::delete_request = (Request *)0x05;

        Session * SessionGroup::delete_session = (Session *)0x01;

        SessionGroup::SessionGroup(
            framework::string::Url const & url, 
            TaskDispatcher & dispatcher)
            : url_(url)
            , dispatcher_(dispatcher)
            , status_(waiting)
            , first_(NULL)
            , current_(NULL)
            , next_(NULL)
            , buffer_finish_(false)
            , canceled_(false)
        {
        }

        Request * SessionGroup::request()
        {
            LOG_XXX("request");

            assert(!busy());
            canceled_ = false;
            assert(current_ == next_);
            if (status_ == waiting) {
                status_ = openning;
                return open_request;
            } else if (current_ == NULL) {
                if (!buffer_finish_) {
                    status_ = bufferring;
                    return buffer_request;
                } else {
                    return NULL;
                }
            } else if (current_ == delete_session) {
                status_ = waiting;
                return delete_request;
            } else {
                Request * req = current_->opened() ? current_->request() : switch_request;
                if (req == NULL) {
                    if (!buffer_finish_) {
                        status_ = bufferring;
                        req = buffer_request;
                    }
                } else {
                    buffer_finish_ = false;
                    status_ = working;
                }
                return req;
            }
        }

        void SessionGroup::response(
            boost::system::error_code const & ec)
        {
            LOG_XXX("response");

            assert(busy());
            canceled_ = false;
            if (status_ == openning) {
                // �ȴ������ڲ�״̬���ٵ��ûص�
                if (current_ && current_ == first_) {
                    current_->response(ec);
                }
                if (ec) {
                    if (current_) {
                        kick_outs_.push_back(current_);
                    }
                    first_ = current_ = next_ = delete_session;
                }
                status_ = openned;
            } else if (status_ == working) {
                assert(current_);
                assert(current_ != delete_session);
                Session * current = current_;
                current_->response(ec);
                if (current_->closed()) {
                    current_->close(boost::asio::error::operation_aborted);
                    delete current_;
                    if (next_ == current_) {
                        next_ = NULL;
                    }
                    current_ = NULL;
                }
                if (next_ != current_) {
                    if (current_ != NULL) {
                        current_->cancel(error::session_kick_out);
                        kick_outs_.push_back(current_);
                    }
                    current_ = next_;
                    boost::system::error_code ec1;
                    current_->response(ec1);
                }
                status_ = openned;
            } else { // status_ == bufferring
                buffer_finish_ = true;
                if (next_ != current_) {
                    if (current_) {
                        current_->cancel(error::session_kick_out);
                        kick_outs_.push_back(current_);
                    }
                    buffer_finish_ = false;
                    current_ = next_;
                }
                status_ = openned;
            }
        }

        void SessionGroup::close(
            boost::system::error_code const & ec)
        {
            LOG_XXX("close");

            assert(!busy());
            if (current_) {
                kick_outs_.push_back(current_);
            }
            if (next_ != current_) {
                kick_outs_.push_back(next_);
            }
            current_ = next_ = NULL;
            for (size_t i = 0; i < kick_outs_.size(); ++i) {
                kick_outs_[i]->close(ec);
                delete kick_outs_[i];
            }
            kick_outs_.clear();
        }

        bool SessionGroup::accept(
            framework::string::Url const & url)
        {
            return url_.param(param_playlink) == url.param(param_playlink) 
                && dispatcher_.accept(url);
        }

        bool SessionGroup::queue_session(
            Session * ses)
        {
            LOG_XXX("queue_session");

            if (status_ == working || status_ == bufferring) {
                if (next_ != current_) {
                    next_->close(error::session_kick_out);
                    kick_outs_.push_back(next_);
                    next_ = ses;
                } else {
                    if (status_ == bufferring) {
                        if (current_ != NULL) {
                            current_->cancel(error::session_kick_out);
                            kick_outs_.push_back(current_);
                            current_ = NULL;
                        }
                        current_ = next_ = ses;
                    } else {
                        next_ = ses;
                    }
                }
                if (canceled_) {
                    return false;
                } else {
                    canceled_ = true;
                    return true;
                }
            } else {
                assert(next_ == current_);
                if (first_ == NULL) {
                    first_ = ses;
                } else if (first_ == current_) {
                    first_ = NULL;
                }
                if (next_) {
                    next_->cancel(error::session_kick_out);
                    kick_outs_.push_back(next_);
                }
                current_ = next_ = ses;
                return status_ == openned;
            }
        }

        bool SessionGroup::active_session(
            Session * ses)
        {
            assert(ready());
            assert(ses == current_);
            if (canceled_) {
                return false;
            } else {
                canceled_ = (status_ == bufferring);
                return canceled_ || status_ == openned;
            }
        }

        bool SessionGroup::close_session(
            Session * ses)
        {
            LOG_XXX("close_session");

            bool need_next = false;
            if (ses == current_) {
                if (status_ == working) {
                    ses->mark_close();
                    need_next = true;
                    if (canceled_) {
                        return false;
                    } else {
                        canceled_ = true;
                        return true;
                    }
                } else {
                    assert(current_ == next_);
                    current_ = next_ = NULL;
                }
            } else if (ses == next_) {
                next_ = current_;
            } else {
                std::vector<Session *>::iterator iter = std::find(
                    kick_outs_.begin(), kick_outs_.end(), ses);
                assert(iter != kick_outs_.end());
                kick_outs_.erase(iter);
            }
            ses->close(boost::asio::error::operation_aborted);
            delete ses;
            if (first_ == ses) {
                first_ = NULL;
            }
            return false;
        }

        Session * SessionGroup::find_session(
            size_t id) const
        {
            if (current_ && current_->id() == id)
                return current_;
            if (next_ && next_->id() == id)
                return next_;
            std::vector<Session *>::const_iterator iter = 
                std::find_if(kick_outs_.begin(), kick_outs_.end(), Session::find_by_id(id));
            return iter == kick_outs_.end() ? NULL : *iter;
        }

    } // namespace dispatch
} // namespace ppbox