// Session.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/Session.h"

#include <boost/bind.hpp>

namespace ppbox
{
    namespace dispatch
    {

        Session::Session(
            boost::asio::io_service & io_svc, 
            framework::string::Url const & url, 
            response_t const & resp)
            : io_svc_(io_svc)
            , url_(url)
            , resp_(resp)
        {
            static boost::uint32_t gid = 0;
            id_ = ++gid;
        }

        Request * Session::request()
        {
            if (play_reqs_.empty())
                return NULL;
            return &play_reqs_.front();
        }

        void Session::response(
            boost::system::error_code const & ec)
        {
            if (!resp_.empty()) {
                response_t resp;
                resp_.swap(resp);
                io_svc_.post(boost::bind(resp, ec));
            } else {
                assert(!play_reqs_.empty());
                response_t resp;
                play_reqs_.front().resp.swap(resp);
                play_reqs_.pop_front();
                io_svc_.post(boost::bind(resp, ec));
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
            boost::system::error_code const & ec)
        {
            response_all(ec);
            id_ = 0;
        }

        void Session::queue_request(
            Request const & r)
        {
            assert(opened());
            assert(!closed());
            play_reqs_.push_back(r);
            play_reqs_.back().session = this;
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

    } // namespace dispatch
} // namespace ppbox
