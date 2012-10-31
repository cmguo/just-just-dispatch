// MuxDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/mux/MuxDispatcher.h"
#include "ppbox/dispatch/mux/MuxTask.h"
#include "ppbox/dispatch/Error.h"

#include <ppbox/mux/MuxModule.h>
#include <ppbox/mux/MuxError.h>

#include <ppbox/demux/DemuxModule.h>
#include <ppbox/demux/base/SegmentDemuxer.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("MuxDispatcher", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

        MuxDispatcher::MuxDispatcher(
            boost::asio::io_service & io_svc, 
            boost::asio::io_service & dispatch_io_svc)
            : Dispatcher(io_svc, dispatch_io_svc)
            , demuxer_module_(util::daemon::use_module<ppbox::demux::DemuxModule>(io_svc))
            , muxer_module_(util::daemon::use_module<ppbox::mux::MuxModule>(io_svc))
            , demux_close_token_(0)
            , demuxer_(NULL)
            , muxer_(NULL)
            , cancel_token_(false)
            , pause_token_(false)
        {
        }

        MuxDispatcher::~MuxDispatcher()
        {
        }

        void MuxDispatcher::start_open(
            framework::string::Url const & url)
        {
            LOG_DEBUG("[start_open] playlink:"<< url.param("playlink"));
            format_ = url.param("format");
            demuxer_module_.async_open(
                framework::string::Url(url.param("playlink")), 
                demux_close_token_, 
                boost::bind(&MuxDispatcher::handle_open, this, _1, _2));
        }

        void MuxDispatcher::handle_open(
            boost::system::error_code const & ec,
            ppbox::demux::SegmentDemuxer * demuxer)
        {
            LOG_DEBUG("[handle_open] ec:" << ec.message());
            demuxer_ = demuxer;
            boost::system::error_code ec1 = ec;
            if (!ec1) {
                open_muxer(ec1);
            }
            response(ec1);
        }

        void MuxDispatcher::cancel_open(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_open]");
            demuxer_module_.close(demux_close_token_,ec);
            demux_close_token_ = 0;
            demuxer_ = NULL;
        }

        void MuxDispatcher::do_setup(
            boost::uint32_t index, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[do_setup]");
            //muxer_->setup(index, ec);
        }

        bool MuxDispatcher::pause(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[pause]");
            pause_token_ = true;
            ec.clear();
            return true;
        }

        bool MuxDispatcher::resume(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[resume]");
            pause_token_ = false;
            ec.clear();
            return true;
        }

        void MuxDispatcher::start_play(
            SeekRange const & range, 
            response_t const & seek_resp)
        {
            LOG_DEBUG("[start_play]");
            dispatch_io_svc().post(MuxTask(io_svc(), sink_group(), range, seek_resp, 
                boost::bind(&MuxDispatcher::handle_play, this, _1), 
                cancel_token_, pause_token_, demuxer_, muxer_));
        }

        void MuxDispatcher::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_play] ec:"<<ec.message());
            cancel_token_ = false;
            pause_token_ = false;
            response(ec);
        }

        void MuxDispatcher::cancel_play(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_play]");
            cancel_token_ = true;
        }

        void MuxDispatcher::start_buffer()
        {
            LOG_DEBUG("[start_buffer]");
            dispatch_io_svc().post(MuxTask(io_svc(), 
                boost::bind(&MuxDispatcher::handle_buffer, this, _1), 
                cancel_token_, demuxer_, muxer_));
        }

        void MuxDispatcher::handle_buffer(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_buffer] ec:"<<ec.message());
            cancel_token_ = false;
            pause_token_ = false;
            response(ec);
        }

        void MuxDispatcher::cancel_buffer(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel_buffer]");
            cancel_token_ = true;
        }

        void MuxDispatcher::do_close(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[do_close]");
            close_muxer(ec);
            if (demux_close_token_) {
                demuxer_module_.close(demux_close_token_, ec);
                demux_close_token_ = 0;
                demuxer_ = NULL;
            }
        }

        bool MuxDispatcher::get_media_info(
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[get_media_info]");
            muxer_->media_info(info);
            ec.clear();
            return true;
        }

        bool MuxDispatcher::get_play_info(
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[get_play_info]");
            return true;
        }

        bool MuxDispatcher::accept(
            framework::string::Url const & url)
        {
            return true;
        }

        bool MuxDispatcher::assign(
            framework::string::Url const & url, 
            boost::system::error_code & ec)
        {
            std::string format = url.param("format");
            ec.clear();
            if (format_ != format) {
                close_muxer(ec);
                format_ = format;
                open_muxer(ec);
            }
            if (!ec) {
                muxer_->reset(ec);
            }
            return !ec;
        }

        void MuxDispatcher::open_muxer(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[open_muxer]");
            muxer_ = muxer_module_.open(
                demuxer_, 
                format_, 
                ec);
        }

        void MuxDispatcher::close_muxer(
            boost::system::error_code& ec)
        {
            LOG_DEBUG("[close_muxer]");
            if (muxer_) {
                muxer_module_.close(muxer_,ec);
                muxer_ = NULL;
            }
            ec.clear();
        }

    } // namespace mux
} // namespace ppbox
