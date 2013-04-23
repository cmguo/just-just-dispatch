// SingleDispatcher.h

#ifndef _PPBOX_DISPATCH_SINGLE_DISPATCHER_H_
#define _PPBOX_DISPATCH_SINGLE_DISPATCHER_H_

#include "ppbox/dispatch/DispatcherBase.h"

namespace ppbox
{
    namespace dispatch
    {

        class TaskDispatcher;
        class DispatchThread;

        class SingleDispatcher
            : public DispatcherBase
        {
        public:
            SingleDispatcher(
                boost::asio::io_service & io_svc);

            virtual ~SingleDispatcher();

        public:
            virtual void async_open(
                framework::string::Url const & url, 
                response_t const & resp);

            virtual bool setup(
                boost::uint32_t index,      // Á÷±àºÅ
                util::stream::Sink & sink, 
                boost::system::error_code & ec); 

            virtual void async_play(
                SeekRange const & range, 
                response_t const & seek_resp,
                response_t const & resp);

            virtual bool pause(
                boost::system::error_code & ec);

            virtual bool resume(
                boost::system::error_code & ec);

            virtual bool get_media_info(
                MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_stream_info(
                std::vector<StreamInfo> & streams, 
                boost::system::error_code & ec);

            virtual bool get_stream_status(
                StreamStatus & info, 
                boost::system::error_code & ec);

            virtual bool cancel(
                boost::system::error_code & ec);

            virtual bool close(
                boost::system::error_code & ec);

        private:
            DispatchThread * thread_;
            TaskDispatcher * dispatcher_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SINGLE_DISPATCHER_H_
