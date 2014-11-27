// SingleDispatcher.h

#ifndef _JUST_DISPATCH_SINGLE_DISPATCHER_H_
#define _JUST_DISPATCH_SINGLE_DISPATCHER_H_

#include "just/dispatch/CustomDispatcher.h"

namespace just
{
    namespace dispatch
    {

        class TaskDispatcher;
        class DispatchThread;

        class SingleDispatcher
            : public CustomDispatcher
        {
        public:
            SingleDispatcher(
                boost::asio::io_service & io_svc);

            virtual ~SingleDispatcher();

        public:
            virtual void async_open(
                framework::string::Url const & url, 
                response_t const & resp);

            virtual bool close(
                boost::system::error_code & ec);
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_SINGLE_DISPATCHER_H_
