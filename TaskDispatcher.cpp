// TaskDispatcher.h

#include "just/dispatch/Common.h"
#include "just/dispatch/TaskDispatcher.h"
#include "just/dispatch/DispatcherTypes.h"
#include "just/dispatch/DispatchModule.h"
#include "just/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>

namespace just
{
    namespace dispatch
    {

        FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.dispatch.TaskDispatcher", framework::logger::Debug);

        TaskDispatcher::TaskDispatcher(
            boost::asio::io_service & io_svc)
            : DispatcherBase(io_svc)
            , module_(util::daemon::use_module<DispatchModule>(io_svc))
            , task_info_(io_svc)
            , async_type_(none)
        {
        }

        TaskDispatcher::~TaskDispatcher()
        {
        }

        void TaskDispatcher::async_open(
            framework::string::Url const & url, 
            response_t const & resp)
        {
            LOG_DEBUG("[async_open] url: " << url.to_string());

            assert(async_type_ == none);

            if (!url.is_valid()) {
                io_svc().post(
                    boost::bind(resp, error::invalid_url));
                return;
            }

            async_type_ = open;
            resp_ = resp;
            task_info_.fast = url.param("dispatch.fast") == "true";
            start_open(url);
        }

        bool TaskDispatcher::setup(
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[setup]");

            do_setup(index, ec);
            if (!ec) {
                sink_group_.setup(index, sink);
            }
            return !ec;
        }

        bool TaskDispatcher::setup(
            SinkGroup const & sink_group, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[setup]");

            if (sink_group.default_sink_is_set()) {
                do_setup(-1, ec);
            } else {
                for (size_t i = 0; i < sink_group.sink_count(); ++i) {
                    if (sink_group.sink_is_set(i)) {
                        do_setup((boost::uint32_t)i, ec);
                        if (ec) {
                            break;
                        }
                    }
                }
            }
            if (!ec) {
                sink_group_ = sink_group;
            }
            return !ec;
        }

        void TaskDispatcher::async_play(
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
        {
            LOG_DEBUG("[async_play]");

            assert(async_type_ == none);
            async_type_ = play;
            resp_ = resp;
            start_play(range, seek_resp);
        }

        bool TaskDispatcher::pause(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[pause]");
            task_pause(ec);
            return true;
        }

        bool TaskDispatcher::resume(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[resume]");
            task_resume(ec);
            return true;
        }

        bool TaskDispatcher::switch_to(
            framework::string::Url const & url, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[switch_to]");
            task_info_.fast = url.param("dispatch.fast") == "true";
            sink_group_.clear();
            ec.clear();
            return true;
        }

        void TaskDispatcher::async_buffer(
            response_t const & resp)
        {
            LOG_DEBUG("[async_buffer]");

            assert(async_type_ == none);
            async_type_ = buffer;
            resp_ = resp;
            start_buffer();
        }

        bool TaskDispatcher::get_stream_status(
            StreamStatus & status, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[get_stream_status]");

            if (async_type_ == none) {
                do_get_stream_status(status, ec);
                return !ec;
            } else {
                status = task_info_.status;
                ec.clear();
                return true;
            }
        }

        bool TaskDispatcher::cancel(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel]");

            assert(async_type_ != none);
            switch (async_type_) {
                case open:
                    cancel_open(ec);
                    break;
                case play:
                    cancel_play(ec);
                    break;
                case buffer:
                    cancel_buffer(ec);
                    break;
                default:
                    break;
            }
            return !ec;
        }

        void TaskDispatcher::cancel_open(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_open]");
            task_cancel(ec);
        }

        void TaskDispatcher::cancel_play(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_play]");
            task_cancel(ec);
        }

        void TaskDispatcher::cancel_buffer(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_buffer]");
            task_cancel(ec);
        }

        void TaskDispatcher::task_cancel(
            boost::system::error_code & ec)
        {
            task_info_.cancel = true;
            ec.clear();
        }

        void TaskDispatcher::task_pause(
            boost::system::error_code & ec)
        {
            task_info_.pause = true;
            ec.clear();
        }

        void TaskDispatcher::task_resume(
            boost::system::error_code & ec)
        {
            task_info_.pause = false;
            ec.clear();
        }

        bool TaskDispatcher::close(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[close]");

            assert(async_type_ == none);
            do_close(ec);
            return !(ec);
        }

        void TaskDispatcher::post_response(
            boost::system::error_code const & ec)
        {
            assert(async_type_ != none);
            task_info_.cancel = false;
            task_info_.pause = false;
            async_type_ = none;
            io_svc().post(
                boost::bind(resp_, ec));
            resp_.clear();
        }

        void TaskDispatcher::response(
            boost::system::error_code const & ec)
        {
            assert(async_type_ != none);
            task_info_.cancel = false;
            task_info_.pause = false;
            async_type_ = none;
            response_t resp;
            resp.swap(resp_);
            resp(ec);
        }

        TaskDispatcher * TaskDispatcherFactory::create(
            boost::asio::io_service & io_svc, 
            framework::string::Url const & url)
        {
            creator_map_type::const_iterator iter = creator_map().begin();
            for (; iter != creator_map().end(); ++iter) {
                TaskDispatcher * dispatcher = iter->second(io_svc);
                if (dispatcher->accept(url))
                    return dispatcher;
                delete dispatcher;
            }
            return NULL;
        }

    } // namespace dispatch
} // namespace just
