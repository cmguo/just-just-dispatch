// Task.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/Task.h"
#include "ppbox/dispatch/Session.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace ppbox
{
    namespace dispatch
    {

        TaskBase::TaskBase(
            TaskConfig & config, 
            response_t const & resp)
            : config_(config)
            , sinks_(*(SinkGroup *)NULL)
            , resp_(resp)
            , buffer_finish_(false)
        {
        }

        TaskBase::TaskBase(
            TaskConfig & config, 
            SinkGroup & sinks, 
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
            : config_(config)
            , sinks_(sinks)
            , range_(range)
            , seek_resp_(seek_resp)
            , resp_(resp)
            , buffer_finish_(false)
        {
        }

        bool TaskBase::write_sample(
            ppbox::avformat::Sample & sample, 
            boost::system::error_code & ec) const
        {
            using boost::asio::buffer_size;

            size_t n = sinks_.write(sample.itrack, sample.data, ec);
            if (n == sample.size) {
                sample.data.clear();
                return true;
            } else {
                sample.size -= n;
                while (n >= buffer_size(sample.data.front())) {
                    n -= buffer_size(sample.data.front());
                    sample.data.pop_front();
                }
                if (n) {
                    sample.data.front() = sample.data.front() + n;
                }
                if (!ec) {
                    ec = boost::asio::error::would_block;
                }
                return false;
            }
        }

        void TaskBase::check_speed(
            ppbox::avformat::Sample const & sample) const
        {
            framework::timer::Time send_time = start_time_ 
                + framework::timer::Duration::milliseconds(sample.time);
            if (send_time > framework::timer::Time::now()) {
                sleep();
            }
        }

        void TaskBase::sleep() const
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }

        void TaskBase::response(
            response_t const & resp, 
            boost::system::error_code const & ec) const
        {
            config_.io_svc.post(boost::bind(resp, ec));
        }

    } // namespace dispatch
} // namespace ppbox
