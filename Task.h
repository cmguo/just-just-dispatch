// Task.h

#ifndef _PPBOX_DISPATCH_TASK_H_
#define _PPBOX_DISPATCH_TASK_H_

#include "ppbox/dispatch/DispatchBase.h"
#include "ppbox/dispatch/SinkGroup.h"

#include <ppbox/avformat/Format.h>

namespace ppbox
{
    namespace dispatch
    {

        class TaskBase
        {
        public:
            TaskBase(
                boost::asio::io_service & io_svc, 
                response_t const & resp, 
                bool const & cancel_token);

            TaskBase(
                boost::asio::io_service & io_svc, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp, 
                bool const & cancel_token, 
                bool const & pause_token);

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

            void sleep() const;

        protected:
            void response(
                response_t const & resp, 
                boost::system::error_code const & ec) const;

        protected:
            boost::asio::io_service & io_svc_;
            SinkGroup & sinks_;
            SeekRange range_;
            response_t seek_resp_;
            response_t resp_;
            bool const & cancel_;
            bool const & pause_;
            ppbox::avformat::Sample sample_;
            bool buffer_finish_;
        };

        template <
            typename TaskImpl
        >
        class Task
            : private TaskBase
        {
        public:
            Task(
                boost::asio::io_service & io_svc, 
                response_t const & resp, 
                bool const & cancel_token)
                : TaskBase(io_svc, resp, cancel_token)
            {
            }

            Task(
                boost::asio::io_service & io_svc, 
                SinkGroup & sinks, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp, 
                bool const & cancel_token, 
                bool const & pause_token)
                : TaskBase(io_svc, sinks, range, seek_resp, resp, cancel_token, pause_token)
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
            bool write_sample_hard(
                TaskImpl & task, 
                ppbox::avformat::Sample & sample, 
                boost::system::error_code & ec)
            {
                if (task.write_sample(sample, ec)) {
                    return true;
                }

                while (!cancel_) {
                    if (!task.write_continuable(ec)) {
                        break;
                    }
                    if (!buffer_finish_) {
                        for (size_t i = 0; i < 10 && !cancel_; ++i) {
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

                if (cancel_)
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

                if (seek_resp_) {
                    response(seek_resp_, ec);
                    seek_resp_.clear();
                }

                if (ec) {
                    response(resp_, ec);
                    return;
                }

                while (!cancel_) {
                    if (pause_) {
                        task.sleep();
                        continue;
                    }

                    if (task.read_sample(sample_, ec)) {
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

                if (cancel_)
                    ec = boost::asio::error::operation_aborted;

                response(resp_, ec);
            }

            void do_buffer()
            {
                TaskImpl & task = static_cast<TaskImpl &>(*this);

                boost::system::error_code ec;

                while (!cancel_ && !buffer_finish_) {
                    for (size_t i = 0; i < 10 && !cancel_; ++i) {
                        if (!task.buffer(ec) && !task.read_continuable(ec)) {
                            buffer_finish_ = true;
                            break;
                        }
                    }
                    if (buffer_finish_)
                        break;
                    task.sleep();
                }

                if (cancel_)
                    ec = boost::asio::error::operation_aborted;

                response(resp_, ec);
            }
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_TASK_H_
