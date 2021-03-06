// MergeDispatcher.cpp

#ifndef JUST_DISABLE_MERGE

#include "just/dispatch/Common.h"
#include "just/dispatch/merge/MergeDispatcher.h"
#include "just/dispatch/merge/MergeTask.h"
#include "just/dispatch/DispatchModule.h"
#include "just/dispatch/DispatchTask.h"
#include "just/dispatch/Error.h"

#include <just/merge/MergeModule.h>
#include <just/merge/MergeError.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.dispatch.MergeDispatcher", framework::logger::Debug);

namespace just
{
    namespace dispatch
    {

        MergeDispatcher::MergeDispatcher(
            boost::asio::io_service & io_svc)
            : TaskDispatcher(io_svc)
            , merge_module_(util::daemon::use_module<just::merge::MergeModule>(io_svc))
            , merge_close_token_(0)
            , merger_(NULL)
            , cancel_token_(false)
            , pause_token_(false)
        {
        }

        MergeDispatcher::~MergeDispatcher()
        {
        }

        void MergeDispatcher::start_open(
            framework::string::Url const & url)
        {
            LOG_DEBUG("[start_open] url:" << url.param(param_url));
            format_ = url.param(param_format);
            boost::system::error_code ec;
            merger_ = merge_module_.create(
                framework::string::Url(url.param(param_url)), 
                url, ec);
            if (merger_) {
                merger_->async_open(
                    boost::bind(&MergeDispatcher::handle_open, this, _1));
            } else {
                post_response(ec);
            }
        }

        void MergeDispatcher::handle_open(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_open] ec:" << ec.message());
            response(ec);
        }

        void MergeDispatcher::cancel_open(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_open]");
            merger_->cancel(ec);
        }

        void MergeDispatcher::do_setup(
            boost::uint32_t index, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[do_setup]");
            //muxer_->setup(index, ec);
            assert(index == (boost::uint32_t)-1);
            ec.clear();
        }

        void MergeDispatcher::start_play(
            SeekRange const & range, 
            response_t const & seek_resp)
        {
            LOG_DEBUG("[start_play]");
            MergeTask task(task_info(), sink_group(), range, seek_resp, 
                boost::bind(&MergeDispatcher::handle_play, this, _1), merger_);
            module().post_thread_task(make_task(task));
        }

        void MergeDispatcher::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_play] ec:" << ec.message());
            boost::system::error_code ec1 = ec;
            if (ec1 == just::merge::error::end_of_file) {
                ec1.clear();
            }
            response(ec1);
        }

        void MergeDispatcher::start_buffer()
        {
            LOG_DEBUG("[start_buffer]");
            MergeTask task(task_info(), 
                boost::bind(&MergeDispatcher::handle_buffer, this, _1), merger_);
            module().post_thread_task(make_task(task));
        }

        void MergeDispatcher::handle_buffer(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_buffer] ec:"<<ec.message());
            response(ec);
        }

        void MergeDispatcher::do_close(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[do_close]");
            if (merger_) {
                merger_->close(ec);
                merge_module_.destroy(merger_, ec);
                merger_ = NULL;
            }
        }

        bool MergeDispatcher::get_media_info(
            MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[get_media_info]");
            merger_->media_info(info);
            ec.clear();
            return true;
        }

        bool MergeDispatcher::get_stream_info(
            std::vector<StreamInfo> & streams, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[get_stream_info]");
            merger_->stream_info(streams);
            ec.clear();
            return true;
        }

        bool MergeDispatcher::get_data_stat(
            DataStat & stat, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[get_stream_info]");
            merger_->data_stat(stat);
            ec.clear();
            return true;
        }

        bool MergeDispatcher::seek(
            SeekRange & range, 
            boost::system::error_code & ec)
        {
            if (range.type == SeekRange::byte) {
                return merger_->byte_seek(range.beg, ec);
            } else {
                ec = framework::system::logic_error::not_supported;
                return false;
            }
        }

        bool MergeDispatcher::read(
            Sample & sample, 
            boost::system::error_code & ec)
        {
            return merger_->read(sample, ec);
        }

        bool MergeDispatcher::free(
            Sample & sample, 
            boost::system::error_code & ec)
        {
            return merger_->free(sample, ec);
        }

        void MergeDispatcher::do_get_stream_status(
            StreamStatus & status, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[do_get_stream_status]");
            merger_->fill_data(ec);
            merger_->stream_status(status);
            ec.clear();
        }

        bool MergeDispatcher::accept(
            framework::string::Url const & url)
        {
            // TO BE FIX
            return url.param(param_format) == "mp4";
        }

        bool MergeDispatcher::switch_to(
            framework::string::Url const & url, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[switch_to]");
            TaskDispatcher::switch_to(url, ec);
            std::string format = url.param(param_format);
            if (!ec && format_ != format) {
                ec = just::merge::error::format_not_match;
            }
            if (!ec) {
                merger_->reset(ec);
                if (ec == boost::asio::error::would_block)
                    ec.clear();
            }
            return !ec;
        }

    } // namespace mux
} // namespace just

#endif
