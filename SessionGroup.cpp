// SessionGroup.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SessionGroup.h"
#include "ppbox/dispatch/Session.h"
#include "ppbox/dispatch/Dispatcher.h"
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
        };

#define LOG_XXX(p) LOG_DEBUG("[" p "] status:" << status_str[status_] << " current:" << current_ << " next:" << next_);

        Request * SessionGroup::open_request = (Request *)0x01;
        Request * SessionGroup::buffer_request = (Request *)0x02;
        Request * SessionGroup::delete_request = (Request *)0x03;

        Session * SessionGroup::buffer_session = (Session *)0x01;
        Session * SessionGroup::delete_session = (Session *)0x02;

        SessionGroup::SessionGroup(
            framework::string::Url const & url, 
            Dispatcher & dispatcher)
            : url_(url)
            , dispatcher_(dispatcher)
            , status_(waiting)
            , current_(NULL)
            , next_(NULL)
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
                return NULL;
            } else if (current_ == buffer_session) {
                status_ = working;
                return buffer_request;
            } else if (current_ == delete_session) {
                status_ = waiting;
                return delete_request;
            } else {
                Request * req = current_->request();
                if (req == NULL) {
                    current_ = next_ = NULL;
                } else {
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
                for (size_t i = 0; i < sessions_.size(); ++i) {
                    sessions_[i]->response(ec);
                }
                if (ec) {
                    current_ = next_ = delete_session;
                } else {
                    current_ = next_ = buffer_session;
                }
            } else {
                assert(current_);
                assert(current_ != delete_session);
                if (current_ != buffer_session) {
                    current_->response(ec);
                    if (current_->closed()) {
                        current_->close(boost::asio::error::operation_aborted);
                        std::vector<Session *>::iterator iter = 
                            std::find(sessions_.begin(), sessions_.end(), current_);
                        assert(iter != sessions_.end());
                        sessions_.erase(iter);
                        if (next_ == current_) {
                            next_ = NULL;
                        }
                        delete current_;
                        current_ = NULL;
                    }
                }
                if (next_ != current_) {
                    if (current_ != buffer_session && current_ != NULL) {
                        current_->cancel(error::session_kick_out); // 回话不立即关闭，还可以接收新的请求
                    }
                    current_ = next_;
                } else if (current_ == buffer_session) {
                    current_ = next_ = NULL;
                } else if (current_ == NULL) {
                    current_ = next_ = buffer_session;
                }
            }
            status_ = openned;
        }

        void SessionGroup::close(
            boost::system::error_code const & ec)
        {
            LOG_XXX("close");

            assert(!busy());
            for (size_t i = 0; i < sessions_.size(); ++i) {
                sessions_[i]->close(ec);
                delete sessions_[i];
            }
            current_ = next_ = NULL;
        }

        bool SessionGroup::accept(
            framework::string::Url const & url)
        {
            return url_.param("playlink") == url.param("playlink") 
                && dispatcher_.accept(url);
        }

        void SessionGroup::queue_session(
            Session * ses)
        {
            LOG_XXX("queue_session");

            sessions_.push_back(ses);
            if (ready()) {
                boost::system::error_code ec;
                ses->response(ec);
            }
        }

        bool SessionGroup::active_session(
            Session * ses)
        {
            LOG_XXX("active_session");

            assert(ready());
            std::vector<Session *>::iterator iter = 
                std::find(sessions_.begin(), sessions_.end(), ses);
            assert(iter != sessions_.end());
            (void)iter;
            bool need_next = (current_ == next_);
            if (current_ == NULL) {
                current_ = next_ = ses;
            } else if (next_ == current_) {
                if (next_ != ses) {
                    next_ = ses;
                } else {
                    need_next = false;
                }
            } else {
                // 为简单起见：如果当前会话在取消中，即使该会话重新激活，也不能再踢其他会话
                if (ses != current_ && ses != next_) {
                    next_ = ses;
                }
            }
            return need_next;
        }

        bool SessionGroup::close_session(
            Session * ses)
        {
            LOG_XXX("close_session");

            std::vector<Session *>::iterator iter = 
                std::find(sessions_.begin(), sessions_.end(), ses);
            assert(iter != sessions_.end());
            bool need_next = (ses != NULL);
            if (ses == current_) {
                ses->mark_close();
            } else {
                if (ses == next_) {
                    next_ = current_;
                }
                ses->close(boost::asio::error::operation_aborted);
                sessions_.erase(iter);
                delete ses;
                need_next = sessions_.empty();
            }
            return need_next;
        }

        Session * SessionGroup::find_session(
            size_t id) const
        {
            struct FindSessionById
            {
                FindSessionById(
                    size_t id)
                    : id_(id)
                {
                }

                bool operator()(
                    Session * s)
                {
                    return s->id() == id_;
                }

            private:
                size_t id_;
            };

            std::vector<Session *>::const_iterator iter = 
                std::find_if(sessions_.begin(), sessions_.end(), FindSessionById(id));
            return iter == sessions_.end() ? NULL : *iter;
        }

    } // namespace dispatch
} // namespace ppbox
