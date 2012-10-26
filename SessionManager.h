// Manager.h

#ifndef _PPBOX_DISPATCH_MANAGER_H_
#define _PPBOX_DISPATCH_MANAGER_H_

#include "ppbox/dispatch/DispatchBase.h"

#include <boost/asio/deadline_timer.hpp>

namespace ppbox
{
    namespace dispatch
    {

        class Session;
        class SessionGroup;
        class DispatchThread;
        class Dispatcher;

        class SessionManager
        {
        public:
            SessionManager(
                boost::asio::io_service & io_svc);

            virtual ~SessionManager();

        public:
            bool start(
                boost::system::error_code & ec);

            bool kill(
                boost::system::error_code & ec);

            bool stop(
                boost::system::error_code & ec);

        public:
            void async_open(
                boost::uint32_t&  sid,      // 会话ID
                framework::string::Url const & playlink, 
                response_t const & resp);

            bool setup(
                boost::uint32_t sid,        // 会话ID
                boost::uint32_t index,      // 流编号
                util::stream::Sink & sink, 
                boost::system::error_code & ec); 

            void async_play(
                boost::uint32_t sid,        // 会话ID
                SeekRange const & range, 
                response_t const & seek_resp,
                response_t const & resp);

            bool cancel(
                boost::uint32_t sid,        // 会话ID
                boost::system::error_code & ec);

            bool pause(
                boost::uint32_t sid,        // 会话ID
                boost::system::error_code & ec);

            bool resume(
                boost::uint32_t sid,        // 会话ID
                boost::system::error_code & ec);

            bool close(
                boost::uint32_t sid,        // 会话ID
                boost::system::error_code & ec);
            
            bool get_media_info(
                boost::uint32_t sid,        // 会话ID
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec);

            bool get_play_info(
                boost::uint32_t sid,        // 会话ID
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec);

        public:
            boost::asio::io_service & io_svc()
            {
                return io_svc_;
            }

        private:
            void handle_request(
                boost::system::error_code const & ec);

            void handle_timer(
                boost::system::error_code const & ec);

            Session * user_session(
                boost::uint32_t sid,        // 会话ID
                boost::system::error_code & ec);

            void async_wait(
                boost::uint32_t wait_timer) ;

            void cancel_wait(
                boost::system::error_code & ec);

        private:
            boost::asio::io_service & io_svc_;
            boost::asio::deadline_timer timer_;
            boost::uint32_t time_id_;

        private:
            DispatchThread * thread_;

        private:
            SessionGroup * current_;
            SessionGroup * next_;
            Session * session_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_MANAGER_H_
