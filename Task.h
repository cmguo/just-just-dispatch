// Task.h

#ifndef _JUST_DISPATCH_TASK_H_
#define _JUST_DISPATCH_TASK_H_

#include "just/dispatch/TaskInfo.h"
#include "just/dispatch/DispatchBase.h"
#include "just/dispatch/SinkGroup.h"

#include <framework/timer/ClockTime.h>

namespace just
{
    namespace dispatch
    {

        class TaskBase
        {
        public:
            TaskBase(
                TaskInfo & info, 
                response_t const & resp);

            TaskBase(
                TaskInfo & info, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp);

        public:
            bool byte_seek(
                boost::uint64_t beg, 
                boost::uint64_t end, 
                boost::system::error_code & ec)
            {
                return false;
            }

            bool time_seek(
                boost::uint64_t beg, 
                boost::uint64_t end, 
                boost::system::error_code & ec)
            {
                return false;
            }

            boost::uint64_t check_seek(
                boost::system::error_code & ec)
            {
                return 0;
            }

            bool read_sample(
                Sample & sample, 
                boost::system::error_code & ec)
            {
                return false;
            }

            bool free_sample(
                Sample & sample, 
                boost::system::error_code & ec)
            {
                return false;
            }

            bool buffer(
                boost::system::error_code & ec)
            {
                return false;
            }

        public:
            bool read_continuable(
                boost::system::error_code const & ec)
            {
                return ec == boost::asio::error::would_block;
            }

            bool write_sample(
                Sample & sample, 
                boost::system::error_code & ec) const;

            bool write_continuable(
                boost::system::error_code const & ec)
            {
                return ec == boost::asio::error::would_block;
            }

            void reset_time(
                boost::uint64_t time);

            void sleep(
                framework::timer::Duration const & d) const;

        protected:
            void response(
                response_t const & resp, 
                boost::system::error_code const & ec) const;

        protected:
            TaskInfo & task_info_;
            SinkGroup & sinks_;
            SeekRange range_;
            response_t seek_resp_;
            response_t resp_;
            Sample sample_;
            bool buffer_finish_;
            framework::timer::Time start_time_;
        };

        template <
            typename TaskImpl
        >
        class Task
            : private TaskBase
        {
        public:
            Task(
                TaskInfo & info, 
                response_t const & resp)
                : TaskBase(info, resp)
            {
            }

            Task(
                TaskInfo & info, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp)
                : TaskBase(info, sinks, range, seek_resp, resp)
            {
            }

        public:
            void operator()()
            {
                if (&sinks_ == NULL)
                    do_buffer();
                else
                    do_play();
            }

        private:
            void check_speed(
                Sample const & sample)
            {
                framework::timer::Time send_time = start_time_ 
                    + framework::timer::Duration::milliseconds(sample.time);
                framework::timer::Time now;
                if (send_time > now) {
                    sleep(send_time - now);
                }
            }

            void sleep(
                framework::timer::Duration const & d)
            {
                TaskImpl & task = static_cast<TaskImpl &>(*this);
                task.update_status(task_info_.status);
                TaskBase::sleep(d);
            }

            void update_status(
                StreamStatus & status)
            {
            }

            bool write_sample_hard(
                TaskImpl & task, 
                Sample & sample, 
                boost::system::error_code & ec)
            {
                if (task.write_sample(sample, ec)) {
                    return true;
                }

                framework::timer::Duration const block_sleep(
                    framework::timer::Duration::milliseconds(100));

                while (!task_info_.cancel) {
                    if (!task.write_continuable(ec)) {
                        break;
                    }
                    if (!buffer_finish_) {
                        for (size_t i = 0; i < 10 && !task_info_.cancel; ++i) {
                            if (!task.buffer(ec) && !task.read_continuable(ec)) {
                                buffer_finish_ = true;
                                break;
                            }
                        }
                    }
                    task.sleep(block_sleep);
                    if (task.write_sample(sample, ec))
                        return true;
                }

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                return false;
            }

        private:
            void do_play()
            {
                TaskImpl & task = static_cast<TaskImpl &>(*this);

                boost::system::error_code ec;

                // �϶�

                framework::timer::Duration const pause_sleep(
                    framework::timer::Duration::milliseconds(100));

                framework::timer::Duration const block_sleep(
                    framework::timer::Duration::milliseconds(100));

                if (range_.type == SeekRange::byte) {
                    task.byte_seek(range_.beg, range_.end, ec);
                } else if (range_.type == SeekRange::time) {
                    task.time_seek(range_.beg, range_.end, ec);
                }

                range_.beg = task.check_seek(ec);
                while (!task_info_.cancel && ec && task.read_continuable(ec)) {
                    task.sleep(block_sleep);
                    range_.beg = task.check_seek(ec);
                }

                reset_time(range_.beg);

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                task.update_status(task_info_.status);

                if (seek_resp_) {
                    task_info_.pause = true;
                    response(seek_resp_, ec);
                    seek_resp_.clear();

                    if (!ec) {
                        while (task_info_.pause) {
                            task.sleep(pause_sleep);
                        }
                    }
                }

                if (ec && !task.read_continuable(ec)) {
                    response(resp_, ec);
                    return;
                }

                while (!task_info_.cancel) {
                    if (task_info_.pause) {
                        task.sleep(pause_sleep);
                        continue;
                    }

                    if (task.read_sample(sample_, ec)) {
                        if (!task_info_.fast)
                            task.check_speed(sample_);
                        if (write_sample_hard(task, sample_, ec)) {
                            // ����
                        } else {
                            break;
                        }
                    } else if (task.read_continuable(ec)) {
                        task.sleep(block_sleep);
                    } else {
                        break;
                    }
                }

                {
                    boost::system::error_code ec1;
                    task.free_sample(sample_, ec1);
                }

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                response(resp_, ec);
            }

            void do_buffer()
            {
                TaskImpl & task = static_cast<TaskImpl &>(*this);

                boost::system::error_code ec;

                framework::timer::Duration const block_sleep(
                    framework::timer::Duration::milliseconds(100));

                while (!task_info_.cancel && !buffer_finish_) {
                    for (size_t i = 0; i < 10 && !task_info_.cancel; ++i) {
                        if (!task.buffer(ec) && !task.read_continuable(ec)) {
                            buffer_finish_ = true;
                            break;
                        }
                    }
                    if (buffer_finish_)
                        break;
                    task.sleep(block_sleep);
                }

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                response(resp_, ec);
            }
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_TASK_H_
