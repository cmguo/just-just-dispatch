// SessionGroup.h

#include "just/dispatch/Common.h"
#include "just/dispatch/SessionGroup.h"
#include "just/dispatch/Session.h"
#include "just/dispatch/TaskDispatcher.h"
#include "just/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

namespace just
{
    namespace dispatch
    {

        FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.dispatch.SessionGroup", framework::logger::Debug);

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
        {
        }

        Request * SessionGroup::request()
        {
            LOG_XXX("request");

            assert(!busy());
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
            if (status_ == openning) {
                // 先处理好内部状态，再调用回调
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
                current_->response(ec);
                if (current_->closed()) {
                    current_->cancel(boost::asio::error::operation_aborted);
                    current_->close(kick_outs_);
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
            boost::system::error_code const & ec, 
            std::vector<Session *> & orphans)
        {
            LOG_XXX("close");

            assert(!busy());
            if (current_ && current_ != delete_session) {
                current_->cancel(ec);
                kick_outs_.push_back(current_);
            }
            if (next_ != current_) {
                next_->cancel(ec);
                kick_outs_.push_back(next_);
            }
            current_ = next_ = NULL;
            orphans.insert(orphans.end(), kick_outs_.begin(), kick_outs_.end());
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
                    next_->cancel(error::session_kick_out);
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
            }
            return status_ != openning;
        }

        bool SessionGroup::active_session(
            Session * ses)
        {
            assert(ready());
            assert(ses == current_);
            return status_ == bufferring || status_ == openned;
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
                    return true;
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
            ses->cancel(boost::asio::error::operation_aborted);
            ses->close(kick_outs_);
            delete ses;
            if (first_ == ses) {
                first_ = NULL;
            }
            return false;
        }

        Session * SessionGroup::find_session(
            size_t id, 
            Session *& main_ses) const
        {
            Session * ses = NULL;
            if (current_ && (ses =  current_->find_sub2(id))) {
                main_ses = current_;
                return ses;
            }
            if (next_ != current_ && (ses = next_->find_sub2(id))) {
                main_ses = next_;
                return ses;
            }
            for (size_t i = 0; i < kick_outs_.size(); ++i) {
                if ((ses = kick_outs_[i]->find_sub2(id))) {
                    main_ses = kick_outs_[i];
                    return ses;
                }
            }
            return NULL;
        }

    } // namespace dispatch
} // namespace just
