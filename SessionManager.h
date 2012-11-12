// Manager.h

#ifndef _PPBOX_DISPATCH_MANAGER_H_
#define _PPBOX_DISPATCH_MANAGER_H_

#include "ppbox/dispatch/DispatchBase.h"

#include <framework/timer/ClockTime.h>

#include <boost/asio/deadline_timer.hpp>

namespace ppbox
{
    namespace dispatch
    {

        class Session;
        class SessionGroup;
        class DispatchThread;
        class TaskDispatcher;

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
                boost::uint32_t&  sid,      // �ỰID
                framework::string::Url const & playlink, 
                response_t const & resp);

            bool setup(
                boost::uint32_t sid,        // �ỰID
                boost::uint32_t index,      // �����
                util::stream::Sink & sink, 
                boost::system::error_code & ec); 

            void async_play(
                boost::uint32_t sid,        // �ỰID
                SeekRange const & range, 
                response_t const & seek_resp,
                response_t const & resp);

            bool cancel(
                boost::uint32_t sid,        // �ỰID
                boost::system::error_code & ec);

            bool pause(
                boost::uint32_t sid,        // �ỰID
                boost::system::error_code & ec);

            bool resume(
                boost::uint32_t sid,        // �ỰID
                boost::system::error_code & ec);

            bool close(
                boost::uint32_t sid,        // �ỰID
                boost::system::error_code & ec);
            
            bool get_media_info(
                boost::uint32_t sid,        // �ỰID
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec);

            bool get_play_info(
                boost::uint32_t sid,        // �ỰID
                ppbox::data::PlayInfo & info, 
                boost::system::error_code & ec);

        public:
            boost::asio::io_service & io_svc()
            {
                return io_svc_;
            }

        private:
            SessionGroup * create_group_with_session(
                boost::uint32_t&  sid,      // �ỰID
                framework::string::Url const & playlink, 
                response_t const & resp);

            void delete_group(
                SessionGroup * group);

            void handle_request(
                boost::system::error_code const & ec);

            void next_request();

            Session * user_session(
                boost::uint32_t sid,        // �ỰID
                boost::system::error_code & ec);

            void start_timer();

            void cancel_timer();

            void handle_timer(
                boost::uint32_t timer_id, 
                boost::system::error_code const & ec);

        private:
            boost::asio::io_service & io_svc_;
            typedef boost::asio::basic_deadline_timer<
                framework::timer::ClockTime> timer_t;
            timer_t timer_;
            boost::uint32_t timer_id_;
            bool timer_lanched_;

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