// TaskDispatcher.h

#ifndef _PPBOX_DISPATCH_TASK_DISPATCHER_H_
#define _PPBOX_DISPATCH_TASK_DISPATCHER_H_

#include "ppbox/dispatch/DispatcherBase.h"
#include "ppbox/dispatch/TaskInfo.h"
#include "ppbox/dispatch/SinkGroup.h"

#include <ppbox/common/ClassFactory.h>

namespace ppbox
{
    namespace dispatch
    {

        struct Request;

        class TaskDispatcher
            : public DispatcherBase
            , public ppbox::common::ClassFactory<
                TaskDispatcher, 
                size_t, 
                TaskDispatcher * (
                    boost::asio::io_service &, 
                    boost::asio::io_service &)
            >
        {
        public:
            static TaskDispatcher * create(
                boost::asio::io_service & io_svc, 
                boost::asio::io_service & dispatch_io_svc, 
                framework::string::Url const & url);

        public:
            TaskDispatcher(
                boost::asio::io_service & io_svc, 
                boost::asio::io_service & dispatch_io_svc);

            virtual ~TaskDispatcher();

        public:
            virtual void async_open(
                framework::string::Url const & playlink, 
                response_t const & resp);

            virtual bool setup(
                boost::uint32_t index,      // Á÷±àºÅ
                util::stream::Sink & sink, 
                boost::system::error_code & ec); 

            virtual void async_play(
                SeekRange const & range, 
                response_t const & seek_resp,
                response_t const & resp);

            virtual bool pause(
                boost::system::error_code & ec);

            virtual bool resume(
                boost::system::error_code & ec);

            virtual bool get_stream_status(
                StreamStatus & status, 
                boost::system::error_code & ec);

            virtual bool cancel(
                boost::system::error_code & ec);

            virtual bool close(
                boost::system::error_code & ec);

        public:
            virtual bool accept(
                framework::string::Url const & url) = 0;

            virtual bool switch_to(
                framework::string::Url const & url, 
                boost::system::error_code & ec);

            virtual void async_buffer(
                response_t const & resp);

            virtual bool setup(
                SinkGroup const & sink_group, 
                boost::system::error_code & ec); 

        protected:
            void response(
                boost::system::error_code const & ec);

            boost::asio::io_service & dispatch_io_svc()
            {
                return dispatch_io_svc_;
            }

            TaskInfo & task_info()
            {
                return task_info_;
            }

            SinkGroup & sink_group()
            {
                return sink_group_;
            }

        private:
            virtual void start_open(
                framework::string::Url const & url) = 0;

            virtual void do_setup(
                boost::uint32_t index, 
                boost::system::error_code & ec) = 0;

            virtual void start_play(
                SeekRange const & range, 
                response_t const & seek_resp) = 0;

            virtual void start_buffer() = 0;

            virtual void cancel_open(
                boost::system::error_code & ec);

            virtual void cancel_play(
                boost::system::error_code & ec);

            virtual void cancel_buffer(
                boost::system::error_code & ec);

            virtual void do_get_stream_status(
                StreamStatus & status, 
                boost::system::error_code & ec) = 0;

            virtual void do_close(
                boost::system::error_code & ec) = 0;

        private:
            void task_cancel(
                boost::system::error_code & ec);

            void task_pause(
                boost::system::error_code & ec);

            void task_resume(
                boost::system::error_code & ec);

        private:
            enum AsyncTypeEnum
            {
                none, 
                open, 
                play, 
                buffer, 
            };

        private:
            boost::asio::io_service & dispatch_io_svc_;
            TaskInfo task_info_;
            AsyncTypeEnum async_type_;
            response_t resp_;
            SinkGroup sink_group_;
        };

    } // namespace dispatch
} // namespace ppbox

#define PPBOX_REGISTER_DISPATCHER(p, c) PPBOX_REGISTER_CLASS(p, c)

#endif // _PPBOX_DISPATCH_TASK_DISPATCHER_H_
