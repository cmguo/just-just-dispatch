// Task.h

#ifndef _PPBOX_DISPATCH_TASK_H_
#define _PPBOX_DISPATCH_TASK_H_

#include "ppbox/dispatch/TaskInfo.h"
#include "ppbox/dispatch/DispatchBase.h"
#include "ppbox/dispatch/SinkGroup.h"

#include <ppbox/avformat/Format.h>

#include <framework/timer/ClockTime.h>

namespace ppbox
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
                ppbox::avformat::Sample & sample, 
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
                ppbox::avformat::Sample & sample, 
                boost::system::error_code & ec) const;

            bool write_continuable(
                boost::system::error_code const & ec)
            {
                return ec == boost::asio::error::would_block;
            }

            void check_speed(
                ppbox::avformat::Sample const & sample) const;

            void reset_time(
                boost::uint64_t time);

            void sleep() const;

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
            ppbox::avformat::Sample sample_;
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
            void sleep()
            {
                TaskImpl & task = static_cast<TaskImpl &>(*this);
                task.update_status(task_info_.status);
                TaskBase::sleep();
            }

            void update_status(
                StreamStatus & status)
            {
            }

            bool write_sample_hard(
                TaskImpl & task, 
                ppbox::avformat::Sample & sample, 
                boost::system::error_code & ec)
            {
                if (task.write_sample(sample, ec)) {
                    return true;
                }

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
                    task.sleep();
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

                // ÍÏ¶¯

                if (range_.type == SeekRange::byte) {
                    task.byte_seek(range_.beg, range_.end, ec);
                } else if (range_.type == SeekRange::time) {
                    task.time_seek(range_.beg, range_.end, ec);
                }

                range_.beg = task.check_seek(ec);
                while (!task_info_.cancel && ec && task.read_continuable(ec)) {
                    task.sleep();
                    range_.beg = task.check_seek(ec);
                }

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                task.update_status(task_info_.status);

                if (seek_resp_) {
                    task_info_.pause = true;
                    response(seek_resp_, ec);
                    seek_resp_.clear();

                    if (!ec) {
                        while (task_info_.pause) {
                            task.sleep();
                        }
                    }
                }

                if (ec && !task.read_continuable(ec)) {
                    response(resp_, ec);
                    return;
                }

                reset_time(range_.beg);

                while (!task_info_.cancel) {
                    if (task_info_.pause) {
                        task.sleep();
                        continue;
                    }

                    if (task.read_sample(sample_, ec)) {
                        if (!task_info_.fast)
                            task.check_speed(sample_);
                        if (write_sample_hard(task, sample_, ec)) {
                            // ÏÞËÙ
                        } else {
                            break;
                        }
                    } else if (task.read_continuable(ec)) {
                        task.sleep();
                    } else {
                        break;
                    }
                }

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                response(resp_, ec);
            }

            void do_buffer()
            {
                TaskImpl & task = static_cast<TaskImpl &>(*this);

                boost::system::error_code ec;

                while (!task_info_.cancel && !buffer_finish_) {
                    for (size_t i = 0; i < 10 && !task_info_.cancel; ++i) {
                        if (!task.buffer(ec) && !task.read_continuable(ec)) {
                            buffer_finish_ = true;
                            break;
                        }
                    }
                    if (buffer_finish_)
                        break;
                    task.sleep();
                }

                if (task_info_.cancel)
                    ec = boost::asio::error::operation_aborted;

                response(resp_, ec);
            }
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_TASK_H_
