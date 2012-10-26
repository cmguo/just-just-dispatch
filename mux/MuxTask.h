// MuxTask.h

#ifndef _PPBOX_DISPATCH_MUX_MUX_TASK_H_
#define _PPBOX_DISPATCH_MUX_MUX_TASK_H_

#include "ppbox/dispatch/Task.h"

#include <ppbox/mux/MuxerBase.h>
#include <ppbox/demux/base/SegmentDemuxer.h>

namespace ppbox
{

    namespace dispatch
    {

        class MuxTask 
            : public Task<MuxTask>
        {
        public:
            MuxTask(
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & resp, 
                bool const & cancel_token, 
                bool const & pause_token, 
                ppbox::demux::SegmentDemuxer* demuxer,
                ppbox::mux::MuxerBase *muxer)
                : Task<MuxTask>(sinks, range, resp, cancel_token, pause_token)
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
                boost::system::error_code ec1;
                demuxer_->get_buffer_time(ec1, ec);
                return !ec;
            }

            //bool byte_seek();

        private:
            ppbox::demux::SegmentDemuxer* demuxer_;
            ppbox::mux::MuxerBase * muxer_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_MUX_MUX_TASK_H_
