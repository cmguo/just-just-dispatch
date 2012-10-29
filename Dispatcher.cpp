// Dispatcher.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/Dispatcher.h"
#include "ppbox/dispatch/mux/MuxDispatcher.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>

namespace ppbox
{
    namespace dispatch
    {

        FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.Dispatcher", framework::logger::Debug);

        std::multimap<size_t, Dispatcher::register_type> & Dispatcher::dispatcher_map()
        {
            static std::multimap<size_t, register_type> g_map;
            return g_map;
        }

        void Dispatcher::register_dispatcher(
            size_t priority, 
            register_type func)
        {
            dispatcher_map().insert(std::make_pair(priority, func));
        }

        Dispatcher * Dispatcher::create(
            boost::asio::io_service & io_svc, 
            boost::asio::io_service & dispatch_io_svc, 
            framework::string::Url const & url)
        {
            std::multimap<size_t, register_type>::const_iterator iter = dispatcher_map().begin();
            for (; iter != dispatcher_map().end(); ++iter) {
                Dispatcher * dispatcher = iter->second(io_svc, dispatch_io_svc);
                if (dispatcher->accept(url))
                    return dispatcher;
                delete dispatcher;
            }
            return NULL;
        }

        void Dispatcher::destory(
            Dispatcher* & dispatcher)
        {
            delete dispatcher;
            dispatcher = NULL;
        }

        Dispatcher::Dispatcher(
            boost::asio::io_service & io_svc, 
            boost::asio::io_service & dispatch_io_svc)
            : DispatcherBase(io_svc)
            , dispatch_io_svc_(dispatch_io_svc)
            , async_type_(none)
        {
        }

        Dispatcher::~Dispatcher()
        {
        }

        void Dispatcher::async_open(
            framework::string::Url const & url, 
            response_t const & resp)
        {
            LOG_DEBUG("[async_open]");

            assert(async_type_ == none);
            async_type_ = open;
            resp_ = resp;
            start_open(url);
        }

        bool Dispatcher::setup(
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[setup]");

            do_setup(index, ec);
            if (!ec) {
                sink_group_.setup(index, sink);
            }
            return !ec;
        }

        bool Dispatcher::setup(
            SinkGroup const & sink_group, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[setup]");

            if (sink_group.default_sink_is_set()) {
                do_setup(-1, ec);
            } else {
                for (size_t i = 0; i < sink_group.sink_count(); ++i) {
                    if (sink_group.sink_is_set(i)) {
                        do_setup((boost::uint32_t)i, ec);
                        if (ec) {
                            break;
                        }
                    }
                }
            }
            if (!ec) {
                sink_group_ = sink_group;
            }
            return !ec;
        }

        void Dispatcher::async_play(
            SeekRange const & range, 
            response_t const & seek_resp, 
            response_t const & resp)
        {
            LOG_DEBUG("[async_play]");

            assert(async_type_ == none);
            async_type_ = play;
            resp_ = resp;
            start_play(range, seek_resp);
        }

        void Dispatcher::async_buffer(
            response_t const & resp)
        {
            LOG_DEBUG("[async_buffer]");

            assert(async_type_ == none);
            async_type_ = buffer;
            resp_ = resp;
            start_buffer();
        }

        bool Dispatcher::cancel(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[cancel]");

            assert(async_type_ != none);
            switch (async_type_) {
                case open:
                    cancel_open(ec);
                    break;
                case play:
                    cancel_play(ec);
                    break;
                case buffer:
                    cancel_buffer(ec);
                    break;
                default:
                    break;
            }
            return !ec;
        }

        bool Dispatcher::close(
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[close]");

            assert(async_type_ == none);
            do_close(ec);
            return !(ec);
        }

        void Dispatcher::response(
            boost::system::error_code const & ec)
        {
            assert(async_type_ != none);
            async_type_ = none;
            response_t resp;
            resp.swap(resp_);
            resp(ec);
        }

    } // namespace dispatch
} // namespace ppbox
