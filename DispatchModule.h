// DispatchModule.h

#ifndef _PPBOX_DISPATCH_DISPATCH_MODULE_H_
#define _PPBOX_DISPATCH_DISPATCH_MODULE_H_

#include <ppbox/common/CommonModuleBase.h>

#include <framework/string/Url.h>

namespace ppbox
{
    namespace dispatch
    {

        class DispatcherBase;
        class SessionManager;

        class DispatchModule
            : public ppbox::common::CommonModuleBase<DispatchModule>
        {
        public:
            DispatchModule(
                util::daemon::Daemon & daemon);

            virtual ~DispatchModule();

        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            DispatcherBase * alloc_dispatcher(
                bool shared = true);

            void free_dispatcher(
                DispatcherBase * dispatcher);

            static bool normalize_url(
                framework::string::Url & url, 
                boost::system::error_code & ec);

        private:
            std::vector<DispatcherBase *> dispatchers_;
            std::vector<SessionManager *> managers_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DISPATCH_MODULE_H_
