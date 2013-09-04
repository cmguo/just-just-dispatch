// DispatchThread.h

#ifndef _PPBOX_DISPATCH_DISPATCH_THREAD_H_
#define _PPBOX_DISPATCH_DISPATCH_THREAD_H_

namespace boost { class thread; }

namespace ppbox
{
    namespace dispatch
    {

        class DispathTaskBase;

        class DispatchThread
        {
        public:
            DispatchThread();

            virtual ~DispatchThread();

        public:
            void post_task(
                DispathTaskBase * task);

        private:
            boost::asio::io_service io_svc_;
            boost::asio::io_service::work * work_;
            boost::thread * thread_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DISPATCH_THREAD_H_
