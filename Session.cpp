// Session.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/Session.h"

namespace ppbox
{
    namespace dispatch
    {

        Session::Session(
            framework::string::Url const & url, 
            response_t const & resp)
            : url_(url)
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
                resp(ec);
            } else {
                assert(!play_reqs_.empty());
                play_reqs_.front().resp(ec);
                play_reqs_.pop_front();
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
                resp(ec);
            } else {
                for (size_t i = 0; i < play_reqs_.size(); ++i) {
                    play_reqs_[i].resp(ec);
                }
                play_reqs_.clear();
            }
            id_ = 0;
        }

    } // namespace dispatch
} // namespace ppbox
