// ThreadDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SingleDispatcher.h"
#include "ppbox/dispatch/DispatchThread.h"
#include "ppbox/dispatch/TaskDispatcher.h"
#include "ppbox/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.ThreadDispatcher", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

        SingleDispatcher::SingleDispatcher(
            boost::asio::io_service& ios)
            : DispatcherBase(ios)
            , thread_(NULL)
            , dispatcher_(NULL)
        {
            thread_ = new DispatchThread;
        }

        SingleDispatcher::~SingleDispatcher()
        {
            if (dispatcher_)
                delete dispatcher_;
            delete thread_;
        }

        void SingleDispatcher::async_open(
            framework::string::Url const & url, 
            response_t const & resp)
        {
            LOG_INFO("[async_open]");

            if (dispatcher_ == NULL) {
                dispatcher_ = TaskDispatcher::create(io_svc(), thread_->io_svc(), url);
            } else if (!dispatcher_->accept(url)) {
                delete dispatcher_;
                dispatcher_ = TaskDispatcher::create(io_svc(), thread_->io_svc(), url);
            }

            if (dispatcher_ == NULL) {
                io_svc().post(boost::bind(resp, error::not_support));
                return;
            }

            return dispatcher_->async_open(url, resp);
        }

        bool SingleDispatcher::setup(
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            return dispatcher_->setup(index, sink, ec);
        }

        void SingleDispatcher::async_play(
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
        {
            return dispatcher_->async_play(range, seek_resp, resp);
        }

        bool SingleDispatcher::pause(
            boost::system::error_code & ec)
        {
            return dispatcher_->pause(ec);
        }

        bool SingleDispatcher::resume(
            boost::system::error_code & ec)
        {
            return dispatcher_->resume(ec);
        }

        bool SingleDispatcher::get_media_info(
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
            return dispatcher_->get_media_info(info, ec);
        }

        bool SingleDispatcher::get_play_info(
            ppbox::data::PlayInfo & info, 
            boost::system::error_code & ec)
        {
            return dispatcher_->get_play_info(info, ec);
        }

        bool SingleDispatcher::cancel(
            boost::system::error_code & ec)
        {
            return dispatcher_->cancel(ec);
        }

        bool SingleDispatcher::close(
            boost::system::error_code & ec)
        {
            return dispatcher_->close(ec);
        }

    } // namespace dispatch
} // namespace ppbox

