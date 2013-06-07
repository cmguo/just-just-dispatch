// DispatchModule.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/DispatchModule.h"
#include "ppbox/dispatch/SessionManager.h"
#include "ppbox/dispatch/SharedDispatcher.h"
#include "ppbox/dispatch/SingleDispatcher.h"

#include <ppbox/common/UrlHelper.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

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
        }

        boost::system::error_code DispatchModule::startup()
        {
            boost::system::error_code ec;
            return ec;
        }

        void DispatchModule::shutdown()
        {
            std::vector<SessionManager *>::iterator iter = managers_.begin();
            for(; iter != managers_.end() ; ++iter) {
                delete (*iter);
            }
        }

        DispatcherBase * DispatchModule::alloc_dispatcher(
            bool shared)
        {
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
            std::vector<DispatcherBase *>::iterator iter = 
                std::find(dispatchers_.begin(), dispatchers_.end(), dispatcher);
            assert(iter != dispatchers_.end());
            if (iter != dispatchers_.end()) {
                dispatchers_.erase(iter);
                delete dispatcher;
            }
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

    } // namespace dispatch
} // namespace ppbox
