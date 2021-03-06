// DispatchThread.cpp

#include "just/dispatch/Common.h"
#include "just/dispatch/DispatchThread.h"
#include "just/dispatch/DispatchTask.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.dispatch.DispatchThread", framework::logger::Debug);

namespace just
{
    namespace dispatch
    {

        DispatchThread::DispatchThread()
        {
            work_ = new boost::asio::io_service::work(io_svc_);
            thread_ = new boost::thread(
                boost::bind(&boost::asio::io_service::run, &io_svc_));
        }

        DispatchThread::~DispatchThread()
        {
            delete work_;
            work_ = NULL;

            thread_->join();
            io_svc_.reset();

            delete thread_;
            thread_ = NULL;
        }

        void DispatchThread::post_task(
            DispathTaskBase * task)
        {
            io_svc_.post(
                boost::bind(&DispathTaskBase::perform, task));
        }


    } // namespace dispatch
} // namespace just

