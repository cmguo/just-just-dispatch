// TaskInfo.h

#ifndef _JUST_DISPATCH_TASK_INFO_H_
#define _JUST_DISPATCH_TASK_INFO_H_

#include "just/dispatch/DispatchBase.h"
#include "just/dispatch/TaskConfig.h"

namespace just
{
    namespace dispatch
    {

        struct TaskInfo
            : TaskConfig
        {
            TaskInfo(
                boost::asio::io_service & io_svc)
                : io_svc(io_svc)
            {
            }

            boost::asio::io_service & io_svc;
            StreamStatus status;
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_TASK_INFO_H_
