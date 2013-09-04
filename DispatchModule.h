// DispatchModule.h

#ifndef _PPBOX_DISPATCH_DISPATCH_MODULE_H_
#define _PPBOX_DISPATCH_DISPATCH_MODULE_H_

#include <ppbox/common/CommonModuleBase.h>

#include <framework/string/Url.h>

#include <boost/thread/mutex.hpp>

namespace ppbox
{
    namespace dispatch
    {

        class DispatcherBase;
        class SessionManager;
        class DispatchThread;
        class DispathTaskBase;

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
            static bool normalize_url(
                framework::string::Url & url, 
                boost::system::error_code & ec);

        public:
            DispatcherBase * alloc_dispatcher(
                bool shared = true);

            void free_dispatcher(
                DispatcherBase * dispatcher);

        public:
            void post_thread_task(
                DispathTaskBase * task);

        public:
            DispatchThread * alloc_thread();

            void free_thread(
                DispatchThread * thread);

        private:
            boost::mutex mutex_;
            std::vector<DispatcherBase *> dispatchers_;
            std::vector<SessionManager *> managers_;
            std::vector<DispatchThread *> threads_; // free only
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DISPATCH_MODULE_H_
