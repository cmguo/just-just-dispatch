// ThreadDispatcher.cpp

#include "just/dispatch/Common.h"
#include "just/dispatch/SingleDispatcher.h"
#include "just/dispatch/DispatchThread.h"
#include "just/dispatch/TaskDispatcher.h"
#include "just/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.dispatch.ThreadDispatcher", framework::logger::Debug);

namespace just
{
    namespace dispatch
    {

        SingleDispatcher::SingleDispatcher(
            boost::asio::io_service & io_svc)
            : CustomDispatcher(io_svc)
        {
        }

        SingleDispatcher::~SingleDispatcher()
        {
            DispatcherBase * dispatcher = detach();
            if (dispatcher)
                delete dispatcher;
        }

        void SingleDispatcher::async_open(
            framework::string::Url const & url, 
            response_t const & resp)
        {
            LOG_INFO("[async_open]");

            DispatcherBase * dispatcher = detach();

            if (dispatcher == NULL) {
                dispatcher = TaskDispatcherFactory::create(io_svc(), url);
            } else if (!((TaskDispatcher *)dispatcher)->accept(url)) {
                delete dispatcher;
                dispatcher = TaskDispatcherFactory::create(io_svc(), url);
            }

            if (dispatcher == NULL) {
                io_svc().post(boost::bind(resp, error::not_support));
                return;
            }

            attach(dispatcher);

            return CustomDispatcher::async_open(url, resp);
        }

        bool SingleDispatcher::close(
            boost::system::error_code & ec)
        {
            DispatcherBase * dispatcher = detach();
            if (dispatcher == NULL) {
                ec = error::session_not_open;
                return false;
            }
            attach(dispatcher);
            return CustomDispatcher::close(ec);
        }

    } // namespace dispatch
} // namespace just

