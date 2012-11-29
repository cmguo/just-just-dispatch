// MuxTask.h

#ifndef _PPBOX_DISPATCH_MUX_MUX_TASK_H_
#define _PPBOX_DISPATCH_MUX_MUX_TASK_H_

#include "ppbox/dispatch/Task.h"

#include <ppbox/mux/MuxerBase.h>
#include <ppbox/demux/base/DemuxerBase.h>

namespace ppbox
{

    namespace dispatch
    {

        class MuxTask 
            : public Task<MuxTask>
        {
        public:
            MuxTask(
                TaskInfo & config, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp, 
                ppbox::demux::DemuxerBase* demuxer,
                ppbox::mux::MuxerBase *muxer)
                : Task<MuxTask>(config, sinks, range, seek_resp, resp)
                , demuxer_(demuxer)
                , muxer_(muxer)
            {
            }
            
            MuxTask(
                TaskInfo & config, 
                response_t const & resp, 
                ppbox::demux::DemuxerBase* demuxer,
                ppbox::mux::MuxerBase *muxer)
                : Task<MuxTask>(config, resp)
                , demuxer_(demuxer)
                , muxer_(muxer)
            {
            }

            bool read_sample(
                ppbox::avformat::Sample & sample, 
                boost::system::error_code & ec)
            {
                muxer_->read(sample, ec);
                return !ec;
            }

            bool buffer(
                boost::system::error_code & ec)
            {
                demuxer_->fill_data(ec);
                return !ec;
            }

            bool byte_seek(
                boost::uint64_t beg, 
                boost::uint64_t end, 
                boost::system::error_code & ec)
            {
                return muxer_->byte_seek(beg, ec);
            }

            bool time_seek(
                boost::uint64_t beg, 
                boost::uint64_t end, 
                boost::system::error_code & ec)
            {
                return muxer_->time_seek(beg, ec);
            }

            boost::uint64_t check_seek(
                boost::system::error_code & ec)
            {
                return demuxer_->check_seek(ec);
            }

            void update_status(
                ppbox::data::StreamStatus & status)
            {
                boost::system::error_code ec;
                demuxer_->get_stream_status(status, ec);
            }

        private:
            ppbox::demux::DemuxerBase* demuxer_;
            ppbox::mux::MuxerBase * muxer_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_MUX_MUX_TASK_H_
