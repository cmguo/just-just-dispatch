// TaskDispatcher.h

#ifndef _PPBOX_DISPATCH_TASK_DISPATCHER_H_
#define _PPBOX_DISPATCH_TASK_DISPATCHER_H_

#include "ppbox/dispatch/DispatcherBase.h"
#include "ppbox/dispatch/TaskInfo.h"
#include "ppbox/dispatch/SinkGroup.h"

#include <util/tools/ClassFactory.h>

namespace ppbox
{
    namespace dispatch
    {

        struct Request;
        class DispatchModule;

        class TaskDispatcher
            : public DispatcherBase
        {
        public:
            TaskDispatcher(
                boost::asio::io_service & io_svc);

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
            void post_response(
                boost::system::error_code const & ec);

            void response(
                boost::system::error_code const & ec);

            DispatchModule & module()
            {
                return module_;
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
            DispatchModule & module_;
            TaskInfo task_info_;
            AsyncTypeEnum async_type_;
            response_t resp_;
            SinkGroup sink_group_;
        };

        struct TaskDispatcherTraits
            : util::tools::ClassFactoryTraits
        {
            typedef boost::uint32_t key_type;
            typedef TaskDispatcher * (create_proto)(
                boost::asio::io_service & io_svc);

            static boost::system::error_code error_not_found();
        };

        class TaskDispatcherFactory
            : public util::tools::ClassFactory<TaskDispatcherTraits>
        {
        public:
            static TaskDispatcher * create(
                boost::asio::io_service & io_svc, 
                framework::string::Url const & url);
        };

    } // namespace dispatch
} // namespace ppbox

#define PPBOX_REGISTER_DISPATCHER(p, c) UTIL_REGISTER_CLASS(ppbox::dispatch::TaskDispatcherFactory, p, c)

#endif // _PPBOX_DISPATCH_TASK_DISPATCHER_H_
