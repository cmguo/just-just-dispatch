// Task.h

#ifndef _JUST_DISPATCH_TASK_CONFIG_H_
#define _JUST_DISPATCH_TASK_CONFIG_H_

namespace just
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
} // namespace just

#endif // _JUST_DISPATCH_TASK_CONFIG_H_
