// CustomDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/CustomDispatcher.h"

namespace ppbox
{
    namespace dispatch
    {

        CustomDispatcher::CustomDispatcher(
            DispatcherBase & dispatcher)
            : DispatcherBase(dispatcher.io_svc())
            , dispatcher_(dispatcher)
        {
        }

        CustomDispatcher::~CustomDispatcher()
        {
        }

        void CustomDispatcher::async_open(
            framework::string::Url const & url, 
            response_t const & resp)
        {
            return dispatcher_.async_open(url, resp);
        }

        bool CustomDispatcher::setup(
            boost::uint32_t index, 
            Sink & sink, 
            boost::system::error_code & ec)
        {
            return dispatcher_.setup(index, sink, ec);
        }

        void CustomDispatcher::async_play(
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
        {
            return dispatcher_.async_play(range, seek_resp, resp);
        }

        bool CustomDispatcher::cancel(
            boost::system::error_code & ec)
        {
            return dispatcher_.cancel(ec);
        }

        bool CustomDispatcher::pause(
            boost::system::error_code & ec)
        {
            return dispatcher_.pause(ec);
        }

        bool CustomDispatcher::resume(
            boost::system::error_code & ec)
        {
            return dispatcher_.resume(ec);
        }

        bool CustomDispatcher::get_media_info(
            MediaInfo & info, 
            boost::system::error_code & ec)
        {
            return dispatcher_.get_media_info(info, ec);
        }

        bool CustomDispatcher::get_stream_info(
            std::vector<StreamInfo> & streams, 
            boost::system::error_code & ec)
        {
            return dispatcher_.get_stream_info(streams, ec);
        }

        bool CustomDispatcher::get_stream_status(
            StreamStatus & info, 
            boost::system::error_code & ec)
        {
            return dispatcher_.get_stream_status(info, ec);
        }

        bool CustomDispatcher::close(
            boost::system::error_code & ec)
        {
            return dispatcher_.close(ec);
        }

    } // namespace dispatch
} // namespace ppbox

