// SharedDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SharedDispatcher.h"
#include "ppbox/dispatch/SessionManager.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.SharedDispatcher", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

        SharedDispatcher::SharedDispatcher(
            SessionManager & manager)
            : DispatcherBase(manager.io_svc())
            , manager_(manager)
            , session_id_(0)
        {
        }

        SharedDispatcher::~SharedDispatcher()
        {
        }

        void SharedDispatcher::async_open(
            framework::string::Url const & url, 
            response_t const & resp)
        {
            return manager_.async_open(session_id_, url, resp);
        }

        bool SharedDispatcher::setup(
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            return manager_.setup(session_id_, index, sink, ec);
        }

        void SharedDispatcher::async_play(
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
        {
            return manager_.async_play(session_id_, range, seek_resp, resp);
        }

        bool SharedDispatcher::cancel(
            boost::system::error_code & ec)
        {
            return manager_.cancel(session_id_, ec);
        }

        bool SharedDispatcher::pause(
            boost::system::error_code & ec)
        {
            return manager_.pause(session_id_, ec);
        }

        bool SharedDispatcher::resume(
            boost::system::error_code & ec)
        {
            return manager_.resume(session_id_, ec);
        }

        bool SharedDispatcher::get_media_info(
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
            return manager_.get_media_info(session_id_, info, ec);
        }

        bool SharedDispatcher::get_play_info(
            ppbox::data::PlayInfo & info, 
            boost::system::error_code & ec)
        {
            return manager_.get_play_info(session_id_, info, ec);
        }

        bool SharedDispatcher::close(
            boost::system::error_code & ec)
        {
            return manager_.close(session_id_, ec);
        }

    } // namespace dispatch
} // namespace ppbox
