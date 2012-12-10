// TaskInfo.h

#ifndef _PPBOX_DISPATCH_TASK_INFO_H_
#define _PPBOX_DISPATCH_TASK_INFO_H_

#include "ppbox/dispatch/DispatchBase.h"
#include "ppbox/dispatch/TaskConfig.h"

namespace ppbox
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
} // namespace ppbox

#endif // _PPBOX_DISPATCH_TASK_INFO_H_
