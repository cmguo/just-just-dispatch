// DispatchThread.h

#ifndef _JUST_DISPATCH_DISPATCH_THREAD_H_
#define _JUST_DISPATCH_DISPATCH_THREAD_H_

namespace boost { class thread; }

namespace just
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
} // namespace just

#endif // _JUST_DISPATCH_DISPATCH_THREAD_H_
