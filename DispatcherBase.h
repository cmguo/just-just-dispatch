// DispatcherBase.h

#ifndef _PPBOX_DISPATCH_DISPATCHER_BASE_H_
#define _PPBOX_DISPATCH_DISPATCHER_BASE_H_

#include "ppbox/dispatch/DispatchBase.h"

namespace ppbox
{
    namespace dispatch
    {

        class DispatcherBase
        {
        public:
            DispatcherBase(
                boost::asio::io_service & io_svc)
                : io_svc_(io_svc)
            {
            }

            virtual ~DispatcherBase() {};

        public:
            virtual void async_open(
                framework::string::Url const & url, 
                response_t const & resp) = 0;

            virtual bool setup(
                boost::uint32_t index,      // Á÷±àºÅ
                util::stream::Sink & sink, 
                boost::system::error_code & ec) = 0; 

            virtual void async_play(
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp) = 0;

            virtual bool cancel(
                boost::system::error_code & ec) = 0;

            virtual bool pause(
                boost::system::error_code & ec) = 0;

            virtual bool resume(
                boost::system::error_code & ec) = 0;

            virtual bool get_media_info(
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec) = 0;

            virtual bool get_play_info(
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec) = 0;

            virtual bool close(
                boost::system::error_code & ec) = 0;

        public:
            boost::asio::io_service & io_svc()
            {
                return io_svc_;
            }

        private:
            boost::asio::io_service & io_svc_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DISPATCHER_BASE_H_
