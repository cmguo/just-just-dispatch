// Task.h

#ifndef _PPBOX_DISPATCH_TASK_CONFIG_H_
#define _PPBOX_DISPATCH_TASK_CONFIG_H_

namespace ppbox
{
    namespace dispatch
    {

        struct TaskConfig
        {
            TaskConfig(
                boost::asio::io_service & io_svc)
                : io_svc(io_svc)
                , fast(false)
                , cancel(false)
                , pause(false)
            {
            }

            boost::asio::io_service & io_svc;
            bool fast;
            bool cancel;
            bool pause;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_TASK_CONFIG_H_
