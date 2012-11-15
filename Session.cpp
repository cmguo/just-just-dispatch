// Session.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/Session.h"

#include <boost/bind.hpp>

namespace ppbox
{
    namespace dispatch
    {

        Request * Session::setup_request = (Request *)0x11;

        Session::Session(
            boost::asio::io_service & io_svc, 
            framework::string::Url const & url, 
            response_t const & resp)
            : io_svc_(io_svc)
            , url_(url)
            , resp_(resp)
            , current_(this)
            , playing_(false)
        {
            static boost::uint32_t gid = 0;
            id_ = ++gid;
        }

        Session::Session(
            boost::asio::io_service & io_svc, 
            framework::string::Url const & url)
            : io_svc_(io_svc)
            , url_(url)
            , resp_(boost::bind(&Session::response_sub_open, this, _1))
            , current_(NULL)
            , playing_(false)
        {
            static boost::uint32_t gid = 0;
            id_ = ++gid;
        }

        Request * Session::request()
        {
            if (play_reqs_.empty()) {
                if (current_ != this) {
                    current_ = NULL;
                }
                return NULL;
            }
            if (current_ != play_reqs_.front().session) {
                current_ = play_reqs_.front().session;
                return setup_request;
            }
            playing_ = true;
            return &play_reqs_.front();
        }

        void Session::response(
            boost::system::error_code const & ec)
        {
            if (!resp_.empty()) {
                response_t resp;
                resp_.swap(resp);
                io_svc_.post(boost::bind(resp, ec));
            } else if (!playing_) {
                // setup response
            } else {
                assert(!play_reqs_.empty());
                assert(play_reqs_.front().session == current_);
                response_t resp;
                play_reqs_.front().resp.swap(resp);
                play_reqs_.pop_front();
                io_svc_.post(boost::bind(resp, ec));
                playing_ = false;
                if (current_ != this && current_->closed()) {
                    close_sub(current_);
                }
            }
        }

        void Session::cancel(
            boost::system::error_code const & ec)
        {
            response_all(ec);
        }

        void Session::mark_close()
        {
            id_ = 0;
        }

        void Session::close(
            std::vector<Session *> & orphans)
        {
            assert(play_reqs_.empty());
            orphans.insert(orphans.end(), sub_sessions_.begin(), sub_sessions_.end());
            sub_sessions_.clear();
            id_ = 0;
        }

        void Session::queue_sub(
            Session * ses)
        {
            sub_sessions_.push_back(ses);
            if (opened()) {
                ses->response(boost::system::error_code());
            }
        }

        bool Session::close_sub(
            Session * ses)
        {
            std::vector<Session *>::iterator iter = 
                std::find(sub_sessions_.begin(), sub_sessions_.end(), ses);
            assert(iter != sub_sessions_.end());
            if (ses == current_ && playing_) {
                ses->mark_close();
                return true;
            } else {
                if (ses == current_) {
                    current_ = NULL;
                }
                sub_sessions_.erase(iter);
                cancel_sub_request(ses, boost::asio::error::operation_aborted);
                ses->cancel(boost::asio::error::operation_aborted);
                ses->close(sub_sessions_);
                delete ses;
                return false;
            }
        }

        Session * Session::find_sub(
            boost::uint32_t id)
        {
            std::vector<Session *>::const_iterator iter = 
                std::find_if(sub_sessions_.begin(), sub_sessions_.end(), find_by_id(id));
            return iter == sub_sessions_.end() ? NULL : *iter;
        }

        void Session::queue_request(
            Request const & r)
        {
            assert(opened());
            assert(!closed());
            play_reqs_.push_back(r);
        }

        void Session::response_all(
            boost::system::error_code const & ec)
        {
            if (!resp_.empty()) {
                response_t resp;
                resp_.swap(resp);
                io_svc_.post(boost::bind(resp, ec));
            } else {
                for (size_t i = 0; i < play_reqs_.size(); ++i) {
                    response_t resp;
                    play_reqs_[i].resp.swap(resp);
                    io_svc_.post(boost::bind(resp, ec));
                }
                play_reqs_.clear();
            }
        }

        void Session::response_sub_open(
            boost::system::error_code const & ec)
        {
            for (size_t i = 0; i < sub_sessions_.size(); ++i) {
                sub_sessions_[i]->response(ec);
            }
        }

        void Session::cancel_sub_request(
            Session * ses, 
            boost::system::error_code const & ec)
        {
            std::deque<Request>::iterator iter = 
                std::remove_if(play_reqs_.begin(), play_reqs_.end(), Request::find_by_session(ses));
            for (; iter != play_reqs_.end(); ++iter) {
                response_t resp;
                iter->resp.swap(resp);
                io_svc_.post(boost::bind(resp, ec));
            }
            play_reqs_.erase(iter, play_reqs_.end());
        }

    } // namespace dispatch
} // namespace ppbox
