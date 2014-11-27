// MergeTask.h

#ifndef _JUST_DISPATCH_MERGE_MERGE_TASK_H_
#define _JUST_DISPATCH_MERGE_MERGE_TASK_H_

#include "just/dispatch/Task.h"

#include <just/merge/MergerBase.h>

namespace just
{

    namespace dispatch
    {

        class MergeTask 
            : public Task<MergeTask>
        {
        public:
            MergeTask(
                TaskInfo & info, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp, 
                just::merge::MergerBase * merger)
                : Task<MergeTask>(info, sinks, range, seek_resp, resp)
                , merger_(merger)
            {
            }
            
            MergeTask(
                TaskInfo & info, 
                response_t const & resp, 
                just::merge::MergerBase * merger)
                : Task<MergeTask>(info, resp)
                , merger_(merger)
            {
            }

            bool read_sample(
                Sample & sample, 
                boost::system::error_code & ec)
            {
                merger_->read(sample, ec);
                return !ec;
            }

            bool free_sample(
                Sample & sample, 
                boost::system::error_code & ec)
            {
                return merger_->free(sample, ec);
            }

            bool buffer(
                boost::system::error_code & ec)
            {
                merger_->fill_data(ec);
                return !ec;
            }

            bool byte_seek(
                boost::uint64_t beg, 
                boost::uint64_t end, 
                boost::system::error_code & ec)
            {
                return merger_->byte_seek(beg, ec);
            }
/*
            bool time_seek(
                boost::uint64_t beg, 
                boost::uint64_t end, 
                boost::system::error_code & ec)
            {
                return merger_->time_seek(beg, ec);
            }
*/
            boost::uint64_t check_seek(
                boost::system::error_code & ec)
            {
                ec.clear();
                return 0;
            }

            void update_status(
                StreamStatus & status)
            {
                merger_->stream_status(status);
            }

        private:
            just::merge::MergerBase * merger_;
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_MERGE_MERGE_TASK_H_
