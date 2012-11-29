// Task.h

#ifndef _PPBOX_DISPATCH_TASK_CONFIG_H_
#define _PPBOX_DISPATCH_TASK_CONFIG_H_

namespace ppbox
{
    namespace dispatch
    {

        struct TaskConfig
        {
            TaskConfig()
                : fast(false)
                , cancel(false)
                , pause(false)
            {
            }

            bool fast;
            bool cancel;
            bool pause;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_TASK_CONFIG_H_
