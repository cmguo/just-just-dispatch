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
            boost::asio::io_service & io_svc, 
            response_t const & resp, 
            bool const & cancel_token)
            : io_svc_(io_svc)
            , sinks_(*(SinkGroup *)NULL)
            , resp_(resp)
            , cancel_(cancel_token)
            , pause_(cancel_token)
            , buffer_finish_(false)
        {
        }

        TaskBase::TaskBase(
            boost::asio::io_service & io_svc, 
            SinkGroup & sinks, 
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp, 
            bool const & cancel_token, 
            bool const & pause_token)
            : io_svc_(io_svc)
            , sinks_(sinks)
            , range_(range)
            , seek_resp_(seek_resp)
            , resp_(resp)
            , cancel_(cancel_token)
            , pause_(pause_token)
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
            } else {
                while (n >= buffer_size(sample.data.front())) {
                    n -= buffer_size(sample.data.front());
                    sample.data.pop_front();
                }
                if (n) {
                    sample.data.front() = sample.data.front() + n;
                }
            }
            return !ec;
        }

        void TaskBase::sleep() const
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }

        void TaskBase::response(
            response_t const & resp, 
            boost::system::error_code const & ec) const
        {
            io_svc_.post(boost::bind(resp, ec));
        }

    } // namespace dispatch
} // namespace ppbox
