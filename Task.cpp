// Task.h

#include "just/dispatch/Common.h"
#include "just/dispatch/Task.h"
#include "just/dispatch/Session.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>

namespace just
{
    namespace dispatch
    {

        TaskBase::TaskBase(
            TaskInfo & task_info, 
            response_t const & resp)
            : task_info_(task_info)
            , sinks_(*(SinkGroup *)NULL)
            , resp_(resp)
            , buffer_finish_(false)
        {
        }

        TaskBase::TaskBase(
            TaskInfo & task_info, 
            SinkGroup & sinks, 
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
            : task_info_(task_info)
            , sinks_(sinks)
            , range_(range)
            , seek_resp_(seek_resp)
            , resp_(resp)
            , buffer_finish_(false)
        {
        }

        bool TaskBase::write_sample(
            Sample & sample, 
            boost::system::error_code & ec) const
        {
            return sinks_.write(sample, ec);
        }

        void TaskBase::reset_time(
            boost::uint64_t time)
        {
            start_time_ = framework::timer::Time::now() - framework::timer::Duration::milliseconds(time);
        }

        void TaskBase::sleep(
            framework::timer::Duration const & d) const
        {
            boost::this_thread::sleep(d.to_posix_duration());
        }

        void TaskBase::response(
            response_t const & resp, 
            boost::system::error_code const & ec) const
        {
            task_info_.io_svc.post(boost::bind(resp, ec));
        }

    } // namespace dispatch
} // namespace just
