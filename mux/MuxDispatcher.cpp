// MuxDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/mux/MuxDispatcher.h"
#include "ppbox/dispatch/mux/MuxTask.h"

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
            , mux_close_token_(0)
            , muxer_(NULL)
            , demuxer_(NULL)
            , cancel_token_(false)
            , pause_token_(false)
        {
        }

        MuxDispatcher::~MuxDispatcher()
        {
        }

        void MuxDispatcher::do_open(
            framework::string::Url const & url)
        {
            LOG_INFO("[do_open] playlink:"<< url.param("playlink"));
            demuxer_module_.async_open(
                framework::string::Url(url.param("playlink")), 
                demux_close_token_, 
                boost::bind(&MuxDispatcher::handle_open, this, _1, _2));
        }

        void MuxDispatcher::handle_open(
            boost::system::error_code const & ec,
            ppbox::demux::SegmentDemuxer * demuxer)
        {
            LOG_INFO("[handle_open] ec:"<<ec.message());
            demuxer_ = demuxer;
            response(ec);
        }

        void MuxDispatcher::cancel_open(
            boost::system::error_code & ec)
        {
            LOG_INFO("[cancel_open_playlink]");
            demuxer_module_.close(demux_close_token_,ec);
            demux_close_token_ = 0;
            demuxer_ = NULL;
        }

        void MuxDispatcher::do_setup(
            boost::uint32_t index, 
            boost::system::error_code & ec)
        {
            //muxer_->setup(index, ec);
        }

        bool MuxDispatcher::pause(
            boost::system::error_code & ec)
        {
            LOG_INFO("[pause]");
            pause_token_ = true;
            ec.clear();
            return true;
        }

        bool MuxDispatcher::resume(
            boost::system::error_code & ec)
        {
            LOG_INFO("[resume]");
            pause_token_ = false;
            ec.clear();
            return true;
        }

        void MuxDispatcher::do_play(
            SeekRange const & range)
        {
            LOG_INFO("[do_play]");
            dispatch_io_svc().post(MuxTask(sink_group(), range, 
                boost::bind(&MuxDispatcher::handle_play, this, _1), 
                cancel_token_, pause_token_, demuxer_, muxer_));
        }

        void MuxDispatcher::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_play] ec:"<<ec.message());
            cancel_token_ = false;
            pause_token_ = false;
            response(ec);
        }

        void MuxDispatcher::cancel_play(
            boost::system::error_code & ec)
        {
            LOG_INFO("[cancel_play_playlink]");
            cancel_token_ = true;
        }

        void MuxDispatcher::do_buffer()
        {
            LOG_INFO("[do_buffer]");
            cancel_token_ = false;
            pause_token_ = false;
            dispatch_io_svc().post(MuxTask(*(SinkGroup *)NULL, SeekRange(), 
                boost::bind(&MuxDispatcher::handle_buffer, this, _1), 
                cancel_token_, pause_token_, demuxer_, muxer_));
        }

        void MuxDispatcher::handle_buffer(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_buffer] ec:"<<ec.message());
            cancel_token_ = false;
            pause_token_ = false;
            response(ec);
        }

        void MuxDispatcher::cancel_buffer(
            boost::system::error_code & ec)
        {
            LOG_INFO("[cancel_buffering]");
            cancel_token_ = true;
        }

        void MuxDispatcher::do_close(
            boost::system::error_code & ec)
        {
            LOG_INFO("[close_playlink]");
            if (mux_close_token_) {
                muxer_module_.close(mux_close_token_, ec);
                mux_close_token_ = 0;
                muxer_ = NULL;
            }
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
            LOG_INFO("[get_media_info]");
            muxer_->media_info(info);
            ec.clear();
            return true;
        }

        bool MuxDispatcher::get_play_info(
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_INFO("[get_play_info]");
            return true;
        }

        bool MuxDispatcher::accept(
            framework::string::Url const & url)
        {
            return true;
        }

/*
        void MuxDispatcher::open_format(std::string const &format,boost::system::error_code& ec)
        {
            LOG_INFO("[open_format]");
            if (format_ != format)
            {
                close_format(ec);
                format_ = format;

                muxer_ = muxer_module_.open(
                    demuxer_,
                    format,
                    mux_close_token_);
            }
        }

        void MuxDispatcher::close_format(boost::system::error_code& ec)
        {
            LOG_INFO("[close_format]");
            if (mux_close_token_)
            {
                muxer_module_.close(mux_close_token_,ec);
                mux_close_token_ = 0;
                muxer_ = NULL;
                format_.clear();
            }
        }

        boost::system::error_code MuxDispatcher::get_sdp(
            std::string & sdp_out,
            boost::system::error_code & ec)
        {
            ((ppbox::mux::RtpMuxer *)muxer_)->get_sdp(sdp_out, ec); 
            return ec;
        }

        boost::system::error_code MuxDispatcher::get_setup(
            boost::uint32_t index,
            std::string & setup_out,
            boost::system::error_code & ec)
        {
            ((ppbox::mux::RtpMuxer *)muxer_)->setup(index, setup_out, ec);
            return ec;
        }

        boost::system::error_code MuxDispatcher::get_rtp_info(
            std::string & rtp_info_out,
            boost::uint32_t & seek_time,
            boost::system::error_code & ec)
        {
            ec = player_->seek(seek_time);
            if (!ec)
            {
                ((ppbox::mux::RtpMuxer *)muxer_)->get_rtp_info(rtp_info_out, seek_time, ec);
            }
            return ec;
        }
*/
    } // namespace mux
} // namespace ppbox
