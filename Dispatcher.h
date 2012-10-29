// Dispatcher.h

#ifndef _PPBOX_DISPATCH_DISPATCHER_H_
#define _PPBOX_DISPATCH_DISPATCHER_H_

#include "ppbox/dispatch/DispatcherBase.h"
#include "ppbox/dispatch/SinkGroup.h"

#include <ppbox/common/Call.h>
#include <ppbox/common/Create.h>

#define PPBOX_REGISTER_DISPATCHER(p, c) \
    static ppbox::common::Call reg_ ## c(ppbox::dispatch::Dispatcher::register_dispatcher, p, ppbox::common::Creator<c>())

namespace ppbox
{
    namespace dispatch
    {

        struct Request;

        class Dispatcher
            : public DispatcherBase
        {
            typedef boost::function<Dispatcher * (
                boost::asio::io_service &, 
                boost::asio::io_service &)
            > register_type;

        public:
            static void register_dispatcher(
                size_t priority, 
                register_type func);

            static Dispatcher * create(
                boost::asio::io_service & io_svc, 
                boost::asio::io_service & dispatch_io_svc, 
                framework::string::Url const & url);

            static void destory(
                Dispatcher* & dispatcher);

        public:
            Dispatcher(
                boost::asio::io_service & io_svc, 
                boost::asio::io_service & dispatch_io_svc);

            virtual ~Dispatcher();

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

            virtual bool cancel(
                boost::system::error_code & ec);

            virtual bool close(
                boost::system::error_code & ec);

        public:
            virtual bool accept(
                framework::string::Url const & url) = 0;

            virtual bool assign(
                framework::string::Url const & url, 
                boost::system::error_code & ec) = 0;

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
                boost::system::error_code & ec) = 0;

            virtual void cancel_play(
                boost::system::error_code & ec) = 0;

            virtual void cancel_buffer(
                boost::system::error_code & ec) = 0;

            virtual void do_close(
                boost::system::error_code & ec) = 0;

        private:
            static std::multimap<size_t, register_type> & dispatcher_map();

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
            AsyncTypeEnum async_type_;
            response_t resp_;
            SinkGroup sink_group_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DISPATCHER_H_
