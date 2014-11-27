// MuxTask.h

#ifndef _JUST_DISPATCH_MUX_MUX_TASK_H_
#define _JUST_DISPATCH_MUX_MUX_TASK_H_

#include "just/dispatch/Task.h"

#include <just/mux/MuxerBase.h>
#include <just/demux/base/DemuxerBase.h>

namespace just
{

    namespace dispatch
    {

        class MuxTask 
            : public Task<MuxTask>
        {
        public:
            MuxTask(
                TaskInfo & info, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp, 
                just::demux::DemuxerBase* demuxer,
                just::mux::MuxerBase *muxer)
                : Task<MuxTask>(info, sinks, range, seek_resp, resp)
                , demuxer_(demuxer)
                , muxer_(muxer)
            {
            }
            
            MuxTask(
                TaskInfo & info, 
                response_t const & resp, 
                just::demux::DemuxerBase* demuxer,
                just::mux::MuxerBase *muxer)
                : Task<MuxTask>(info, resp)
                , demuxer_(demuxer)
                , muxer_(muxer)
            {
            }

            bool read_sample(
                Sample & sample, 
                boost::system::error_code & ec)
            {
                return muxer_->read(sample, ec);
            }

            bool free_sample(
                Sample & sample, 
                boost::system::error_code & ec)
            {
                return demuxer_->free_sample(sample, ec);
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
                return muxer_->check_seek(ec);
            }

            void update_status(
                StreamStatus & status)
            {
                muxer_->stream_status(status);
            }

        private:
            just::demux::DemuxerBase* demuxer_;
            just::mux::MuxerBase * muxer_;
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_MUX_MUX_TASK_H_
