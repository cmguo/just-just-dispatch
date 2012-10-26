// SessionGroup.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SessionGroup.h"
#include "ppbox/dispatch/Session.h"
#include "ppbox/dispatch/Dispatcher.h"

namespace ppbox
{
    namespace dispatch
    {

        Request * SessionGroup::open_request = (Request *)0x01;
        Request * SessionGroup::buffer_request = (Request *)0x02;

        Session * SessionGroup::buffer_session = (Session *)0x01;

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
            assert(!busy());
            assert(current_ == next_);
            if (status_ == waiting) {
                status_ = openning;
                return open_request;
            } else if (current_ == buffer_session) {
                current_ = next_ = NULL;
                return NULL;
            } else if (current_->request()) {
                status_ = working;
                return current_->request();
            } else {
                current_ = next_ = buffer_session;
                return buffer_request;
            }
        }

        void SessionGroup::response(
            boost::system::error_code const & ec)
        {
            assert(busy());
            if (status_ == openning) {
                for (size_t i = 0; i < sessions_.size(); ++i) {
                    sessions_[i]->response(ec);
                }
            } else {
                assert(current_);
                if (current_ != buffer_session) {
                    current_->response(ec);
                    if (current_->closed()) {
                        current_->close(boost::asio::error::operation_aborted);
                    }
                }
                if (next_ != current_) {
                    if (current_ != buffer_session) {
                        current_->cancel(); // 回话不立即关闭，还可以接收新的请求
                    }
                    current_ = next_;
                }
            }
            status_ = openned;
        }

        void SessionGroup::close(
            boost::system::error_code const & ec)
        {
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
            Session * s)
        {
            sessions_.push_back(s);
        }

        bool SessionGroup::active_session(
            Session * s)
        {
            assert(status_ == openned && status_ == working);
            std::vector<Session *>::iterator iter = 
                std::find(sessions_.begin(), sessions_.end(), s);
            assert(iter != sessions_.end());
            (void)iter;
            bool need_cancel = (current_ != NULL && current_ == next_);
            if (current_ == NULL) {
                current_ = next_ = s;
            } else if (s != next_) {
                if (next_ != current_)
                    next_->cancel();
                next_ = s;
            }
            return need_cancel;
        }

        bool SessionGroup::close_session(
            Session * s)
        {
            std::vector<Session *>::iterator iter = 
                std::find(sessions_.begin(), sessions_.end(), s);
            assert(iter != sessions_.end());
            if (s == current_) {
                s->mark_close();
            } else {
                if (s == next_) {
                    next_ = current_;
                    }
                s->close(boost::asio::error::operation_aborted);
                delete s;
                s = NULL;
                sessions_.erase(iter);
            }
            return s != NULL;
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
