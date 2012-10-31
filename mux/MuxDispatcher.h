// MuxDispatcher.h

#ifndef _PPBOX_DISPATCH_MUX_MUX_DISPATCHER_H_
#define _PPBOX_DISPATCH_MUX_MUX_DISPATCHER_H_

#include "ppbox/dispatch/Dispatcher.h"

namespace ppbox
{
    namespace demux
    {
        class SegmentDemuxer;
        class DemuxModule;
    }

    namespace mux
    {
        class MuxModule;
        class MuxerBase;
    }

    namespace dispatch
    {

        class MuxDispatcher 
            : public Dispatcher
        {
        public:
            MuxDispatcher(
                boost::asio::io_service & io_svc, 
                boost::asio::io_service & dispatch_io_svc);

            virtual ~MuxDispatcher();

        public:
            virtual bool pause(
                boost::system::error_code & ec);

            virtual bool resume(
                boost::system::error_code & ec);

            virtual bool get_media_info(
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_play_info(
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec);

        public:
            virtual bool accept(
                framework::string::Url const & url);

            virtual bool assign(
                framework::string::Url const & url, 
                boost::system::error_code & ec);

        private:
            virtual void start_open(
                framework::string::Url const & url);

            virtual void do_setup(
                boost::uint32_t index,      // 流编号
                boost::system::error_code & ec); 

            virtual void start_play(
                SeekRange const & range, 
                response_t const & seek_resp);

            virtual void start_buffer();

            virtual void cancel_open(
                boost::system::error_code & ec);

            virtual void cancel_play(
                boost::system::error_code & ec);

            virtual void cancel_buffer(
                boost::system::error_code & ec);

            virtual void do_close(
                boost::system::error_code & ec);

       private:
            void handle_open(
                boost::system::error_code const & ec, 
                ppbox::demux::SegmentDemuxer * demuxer);

            void handle_play(
                boost::system::error_code const & ec);

            void handle_buffer(
                boost::system::error_code const & ec);

            void open_muxer(
                boost::system::error_code & ec);

            void close_muxer(
                boost::system::error_code & ec);

        private:
            ppbox::demux::DemuxModule & demuxer_module_;
            ppbox::mux::MuxModule & muxer_module_;

            size_t  demux_close_token_;      
            ppbox::demux::SegmentDemuxer* demuxer_;
            ppbox::mux::MuxerBase *muxer_;

            std::string format_;

            bool cancel_token_;
            bool pause_token_;

            boost::uint32_t video_type_;          //视频索引值
            boost::uint32_t audio_type_;          //音频索引值
        };

        PPBOX_REGISTER_DISPATCHER(2, MuxDispatcher);

    } // namespace dispatcher
} // namespace ppbox

#endif // _PPBOX_DISPATCH_MUX_MUX_DISPATCHER_H_
