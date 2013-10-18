// MuxDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/mux/MuxDispatcher.h"
#include "ppbox/dispatch/mux/MuxTask.h"
#include "ppbox/dispatch/DispatchModule.h"
#include "ppbox/dispatch/DispatchTask.h"
#include "ppbox/dispatch/Error.h"

#include <ppbox/mux/MuxModule.h>
#include <ppbox/mux/MuxError.h>

#include <ppbox/demux/DemuxModule.h>
#include <ppbox/demux/base/DemuxerBase.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.MuxDispatcher", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

        MuxDispatcher::MuxDispatcher(
            boost::asio::io_service & io_svc)
            : TaskDispatcher(io_svc)
            , demuxer_module_(util::daemon::use_module<ppbox::demux::DemuxModule>(io_svc))
            , muxer_module_(util::daemon::use_module<ppbox::mux::MuxModule>(io_svc))
            , demuxer_(NULL)
            , muxer_(NULL)
        {
        }

        MuxDispatcher::~MuxDispatcher()
        {
        }

        void MuxDispatcher::start_open(
            framework::string::Url const & url)
        {
            LOG_DEBUG("[start_open] playlink:" << url.param(param_playlink) << " format:" << url.param(param_format));
            boost::system::error_code ec;
            demuxer_ = demuxer_module_.create(
                framework::string::Url(url.param(param_playlink)), 
                url, 
                ec);
            if (demuxer_) {
                demuxer_->async_open(
                    boost::bind(&MuxDispatcher::handle_open, this, url, _1));
            } else {
                post_response(ec);
            }
        }

        void MuxDispatcher::handle_open(
            framework::string::Url const & url, 
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_open] ec:" << ec.message());
            boost::system::error_code ec1 = ec;
            if (!ec1) {
                format_ = url.param(param_format);
                open_muxer(url, ec1);
            }
            response(ec1);
        }

        void MuxDispatcher::cancel_open(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_open]");
            demuxer_->cancel(ec);
        }

        void MuxDispatcher::do_setup(
            boost::uint32_t index, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[do_setup]");
            muxer_->setup(index, ec);
        }

        void MuxDispatcher::start_play(
            SeekRange const & range, 
            response_t const & seek_resp)
        {
            LOG_DEBUG("[start_play]");
            MuxTask task(task_info(), sink_group(), range, seek_resp, 
                boost::bind(&MuxDispatcher::handle_play, this, _1), demuxer_, muxer_);
            module().post_thread_task(make_task(task));
        }

        void MuxDispatcher::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_play] ec:" << ec.message());
            boost::system::error_code ec1 = ec;
            if (ec1 == ppbox::avformat::error::end_of_stream) {
                ec1.clear();
            }
            response(ec1);
        }

        void MuxDispatcher::start_buffer()
        {
            LOG_DEBUG("[start_buffer]");
            MuxTask task(task_info(), 
                boost::bind(&MuxDispatcher::handle_buffer, this, _1), 
                demuxer_, muxer_);
            module().post_thread_task(make_task(task));
        }

        void MuxDispatcher::handle_buffer(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_buffer] ec:"<<ec.message());
            response(ec);
        }

        void MuxDispatcher::do_close(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[do_close]");
            close_muxer(ec);
            if (demuxer_) {
                demuxer_->close(ec);
                demuxer_module_.destroy(demuxer_, ec);
                demuxer_ = NULL;
            }
        }

        bool MuxDispatcher::get_media_info(
            MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[get_media_info]");
            muxer_->media_info(info);
            ec.clear();
            return true;
        }

        bool MuxDispatcher::get_stream_info(
            std::vector<StreamInfo> & streams, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[get_stream_info]");
            muxer_->stream_info(streams);
            ec.clear();
            return true;
        }

        bool MuxDispatcher::get_data_stat(
            DataStat & stat, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[get_stream_info]");
            return demuxer_->get_data_stat(stat, ec);
        }

        bool MuxDispatcher::seek(
            SeekRange & range, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[seek]");
            if (range.type == SeekRange::byte) {
                return muxer_->byte_seek(range.beg, ec);
            } else {
                return muxer_->time_seek(range.beg, ec);
            }
        }

        bool MuxDispatcher::read(
            Sample & sample, 
            boost::system::error_code & ec)
        {
            //LOG_TRACE("[read]");
            return muxer_->read(sample, ec);
        }

        void MuxDispatcher::do_get_stream_status(
            StreamStatus & status, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[do_get_stream_status]");
            demuxer_->fill_data(ec);
            muxer_->stream_status(status);
            ec.clear();
        }

        bool MuxDispatcher::accept(
            framework::string::Url const & url)
        {
            return true;
        }

        bool MuxDispatcher::switch_to(
            framework::string::Url const & url, 
            boost::system::error_code & ec)
        {
            TaskDispatcher::switch_to(url, ec);
            std::string format = url.param(param_format);
            LOG_DEBUG("[assign] format:" << format);
            if (!ec && format_ != format) {
                close_muxer(ec);
                format_ = format;
                open_muxer(url, ec);
            }
            if (!ec) {
                muxer_->reset(ec);
                if (ec == boost::asio::error::would_block)
                    ec.clear();
            }
            return !ec;
        }

        void MuxDispatcher::open_muxer(
            framework::string::Url const & config, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[open_muxer]");
            muxer_ = muxer_module_.open(
                demuxer_, 
                config, 
                ec);
        }

        void MuxDispatcher::close_muxer(
            boost::system::error_code& ec)
        {
            LOG_DEBUG("[close_muxer]");
            if (muxer_) {
                muxer_module_.close(muxer_, ec);
                muxer_ = NULL;
            }
            ec.clear();
        }

    } // namespace mux
} // namespace ppbox
