// MuxDispatcher.h

#ifndef _JUST_DISPATCH_MUX_MUX_DISPATCHER_H_
#define _JUST_DISPATCH_MUX_MUX_DISPATCHER_H_

#include "just/dispatch/TaskDispatcher.h"

namespace just
{
    namespace demux
    {
        class DemuxerBase;
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
            : public TaskDispatcher
        {
        public:
            MuxDispatcher(
                boost::asio::io_service & io_svc);

            virtual ~MuxDispatcher();

        public:
            virtual bool get_media_info(
                MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_stream_info(
                std::vector<StreamInfo> & streams, 
                boost::system::error_code & ec);

            virtual bool get_data_stat(
                DataStat & stat, 
                boost::system::error_code & ec);

        public:
            virtual bool seek(
                SeekRange & range, 
                boost::system::error_code & ec);

            virtual bool read(
                Sample & sample, 
                boost::system::error_code & ec);

            virtual bool free(
                Sample & sample, 
                boost::system::error_code & ec);

        public:
            virtual bool accept(
                framework::string::Url const & url);

            virtual bool switch_to(
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

            virtual void do_get_stream_status(
                StreamStatus & status, 
                boost::system::error_code & ec);

            virtual void do_close(
                boost::system::error_code & ec);

       private:
            void handle_open(
                framework::string::Url const & url, 
                boost::system::error_code const & ec);

            void handle_play(
                boost::system::error_code const & ec);

            void handle_buffer(
                boost::system::error_code const & ec);

            void open_muxer(
                framework::string::Url const & config, 
                boost::system::error_code & ec);

            void close_muxer(
                boost::system::error_code & ec);

        private:
            just::demux::DemuxModule & demuxer_module_;
            just::mux::MuxModule & muxer_module_;

            just::demux::DemuxerBase* demuxer_;
            just::mux::MuxerBase *muxer_;

            std::string format_;

            boost::uint32_t video_type_;          //视频索引值
            boost::uint32_t audio_type_;          //音频索引值
        };

        JUST_REGISTER_DISPATCHER(2, MuxDispatcher);

    } // namespace dispatcher
} // namespace just

#endif // _JUST_DISPATCH_MUX_MUX_DISPATCHER_H_
