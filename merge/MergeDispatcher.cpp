// MergeDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/merge/MergeDispatcher.h"
#include "ppbox/dispatch/merge/MergeTask.h"
#include "ppbox/dispatch/DispatchModule.h"
#include "ppbox/dispatch/DispatchTask.h"
#include "ppbox/dispatch/Error.h"

#include <ppbox/merge/MergeModule.h>
#include <ppbox/merge/MergeError.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.MergeDispatcher", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

        MergeDispatcher::MergeDispatcher(
            boost::asio::io_service & io_svc)
            : TaskDispatcher(io_svc)
            , merge_module_(util::daemon::use_module<ppbox::merge::MergeModule>(io_svc))
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
            LOG_DEBUG("[start_open] playlink:" << url.param(param_playlink));
            format_ = url.param(param_format);
            boost::system::error_code ec;
            merger_ = merge_module_.create(
                framework::string::Url(url.param(param_playlink)), 
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
            if (ec1 == ppbox::merge::error::end_of_file) {
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
            TaskDispatcher::switch_to(url, ec);
            std::string format = url.param(param_format);
            if (!ec && format_ != format) {
                ec = ppbox::merge::error::format_not_match;
            }
            if (!ec) {
                merger_->reset(ec);
            }
            return !ec;
        }

    } // namespace mux
} // namespace ppbox
