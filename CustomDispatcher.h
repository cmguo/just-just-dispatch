// CustomDispatcher.h

#ifndef _PPBOX_DISPATCH_CUSTOM_DISPATCHER_H_
#define _PPBOX_DISPATCH_CUSTOM_DISPATCHER_H_

#include "ppbox/dispatch/DispatcherBase.h"

namespace ppbox
{
    namespace dispatch
    {

        class SessionManager;

        class CustomDispatcher
            : public DispatcherBase
        {
        public:
            CustomDispatcher(
                DispatcherBase & dispatcher);

            virtual ~CustomDispatcher();

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

            virtual bool cancel(
                boost::system::error_code & ec);

            virtual bool pause(
                boost::system::error_code & ec);

            virtual bool resume(
                boost::system::error_code & ec);

            virtual bool close(
                boost::system::error_code & ec);
            
            virtual bool get_media_info(
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_stream_status(
                ppbox::data::StreamStatus & info, 
                boost::system::error_code & ec);

        protected:
            DispatcherBase & dispatcher_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_CUSTOM_DISPATCHER_H_
