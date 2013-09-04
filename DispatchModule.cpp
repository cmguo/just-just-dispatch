// DispatchModule.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/DispatchModule.h"
#include "ppbox/dispatch/SessionManager.h"
#include "ppbox/dispatch/DispatchTask.h"
#include "ppbox/dispatch/DispatchThread.h"
#include "ppbox/dispatch/SharedDispatcher.h"
#include "ppbox/dispatch/SingleDispatcher.h"

#include <ppbox/common/UrlHelper.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("DispatchModule", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

        DispatchModule::DispatchModule(
            util::daemon::Daemon & daemon)
            : ppbox::common::CommonModuleBase<DispatchModule>(daemon, "DispatchModule")
        {
        }

        DispatchModule::~DispatchModule()
        {
            for(size_t i = 0; i < dispatchers_.size() ; ++i) {
                delete dispatchers_[i];
            }
            dispatchers_.clear();
            for(size_t i = 0; i < managers_.size() ; ++i) {
                delete managers_[i];
            }
            managers_.clear();
            for(size_t i = 0; i < threads_.size() ; ++i) {
                delete threads_[i];
            }
            threads_.clear();
        }

        boost::system::error_code DispatchModule::startup()
        {
            boost::system::error_code ec;
            return ec;
        }

        void DispatchModule::shutdown()
        {
        }

        bool DispatchModule::normalize_url(
            framework::string::Url & url, 
            boost::system::error_code & ec)
        {
            ppbox::common::decode_url(url, ec);
            if (ec) {
                return false;
            }

            std::string::size_type pos = url.path().find('.');
            if (pos != std::string::npos) {
                if (url.param(param_format).empty()) {
                    url.param(param_format, url.path().substr(pos + 1));
                }
            }
            if (url.param(param_format).empty()) {
                ec = framework::system::logic_error::invalid_argument;
            }
            return !ec;
        }

        DispatcherBase * DispatchModule::alloc_dispatcher(
            bool shared)
        {
            boost::mutex::scoped_lock lc(mutex_);
            DispatcherBase * dispatcher = NULL;
            if (shared) {
                if (managers_.empty()) {
                    managers_.push_back(new SessionManager(io_svc()));
                }
                SessionManager * manager = managers_[0];
                dispatcher = new SharedDispatcher(*manager);
            } else {
                dispatcher = new SingleDispatcher(io_svc());
            }
            if (dispatcher) {
                dispatchers_.push_back(dispatcher);
            }
            return dispatcher;
        }

        void DispatchModule::free_dispatcher(
            DispatcherBase * dispatcher)
        {
            boost::mutex::scoped_lock lc(mutex_);
            std::vector<DispatcherBase *>::iterator iter = 
                std::find(dispatchers_.begin(), dispatchers_.end(), dispatcher);
            assert(iter != dispatchers_.end());
            if (iter != dispatchers_.end()) {
                dispatchers_.erase(iter);
                delete dispatcher;
            }
        }

        static void do_task(
            DispatchModule * module, 
            DispatchThread * thread, 
            DispathTaskBase * task)
        {
            task->perform();
            module->free_thread(thread);
        }

        void DispatchModule::post_thread_task(
            DispathTaskBase * task)
        {
            DispatchThread * thread = alloc_thread();
            thread->post_task(
                make_task(boost::bind(do_task, this, thread, task)));
        }

        DispatchThread * DispatchModule::alloc_thread()
        {
            boost::mutex::scoped_lock lc(mutex_);
            if (threads_.empty()) {
                return new DispatchThread;
            }
            DispatchThread * thread = threads_.back();
            threads_.pop_back();
            return thread;
        }

        void DispatchModule::free_thread(
            DispatchThread * thread)
        {
            boost::mutex::scoped_lock lc(mutex_);
            threads_.push_back(thread);
        }

    } // namespace dispatch
} // namespace ppbox
