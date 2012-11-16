// MuxDispatcher.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/mux/MuxDispatcher.h"
#include "ppbox/dispatch/mux/MuxTask.h"
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
            boost::asio::io_service & io_svc, 
            boost::asio::io_service & dispatch_io_svc)
            : TaskDispatcher(io_svc, dispatch_io_svc)
            , demuxer_module_(util::daemon::use_module<ppbox::demux::DemuxModule>(io_svc))
            , muxer_module_(util::daemon::use_module<ppbox::mux::MuxModule>(io_svc))
            , demux_close_token_(0)
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
            LOG_DEBUG("[start_open] playlink:"<< url.param(param_playlink) << " format:" << url.param(param_format));
            demuxer_module_.async_open(
                framework::string::Url(url.param(param_playlink)), 
                demux_close_token_, 
                boost::bind(&MuxDispatcher::handle_open, this, url, _1, _2));
        }

        void MuxDispatcher::handle_open(
            framework::string::Url const & url, 
            boost::system::error_code const & ec,
            ppbox::demux::DemuxerBase * demuxer)
        {
            LOG_DEBUG("[handle_open] ec:" << ec.message());
            demuxer_ = demuxer;
            boost::system::error_code ec1 = ec;
            if (!ec1) {
                format_ = url.param(param_format);
                open_muxer(ec1);
                if (muxer_) {
                    framework::string::Url::param_const_iterator iter = url.param_begin();
                    for (; iter != url.param_end(); ++iter) {
                        if (iter->key().compare(0, 4, "mux.") == 0) {
                            std::string::size_type pos_dot = iter->key().rfind('.');
                            if (pos_dot == 3)
                                continue;
                            muxer_->config().set(
                                iter->key().substr(4, pos_dot - 4), 
                                iter->key().substr(pos_dot + 1), 
                                iter->value());
                        }
                    }
                }
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
            muxer_->setup(index, ec);
        }

        void MuxDispatcher::start_play(
            SeekRange const & range, 
            response_t const & seek_resp)
        {
            LOG_DEBUG("[start_play]");
            dispatch_io_svc().post(MuxTask(config(), sink_group(), range, seek_resp, 
                boost::bind(&MuxDispatcher::handle_play, this, _1), demuxer_, muxer_));
        }

        void MuxDispatcher::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_play] ec:" << ec.message());
            boost::system::error_code ec1 = ec;
            if (ec1 == ppbox::mux::error::end_of_stream) {
                ec1.clear();
            }
            response(ec1);
        }

        void MuxDispatcher::start_buffer()
        {
            LOG_DEBUG("[start_buffer]");
            dispatch_io_svc().post(MuxTask(config(), 
                boost::bind(&MuxDispatcher::handle_buffer, this, _1), 
                demuxer_, muxer_));
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
            ppbox::data::PlayInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[get_play_info]");
            muxer_->play_info(info);
            ec.clear();
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
            TaskDispatcher::assign(url, ec);
            std::string format = url.param(param_format);
            LOG_DEBUG("[assign] format:" << format);
            if (format_ != format) {
                close_muxer(ec);
                format_ = format;
                open_muxer(ec);
            }
            if (!ec) {
                muxer_->reset(ec);
                if (ec == boost::asio::error::would_block)
                    ec.clear();
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
