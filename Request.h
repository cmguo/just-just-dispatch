// Request.h

#ifndef _PPBOX_DISPATCH_REQUEST_H_
#define _PPBOX_DISPATCH_REQUEST_H_

#include "ppbox/dispatch/DispatchBase.h"

namespace ppbox
{
    namespace dispatch
    {

        class Session;

        struct Request
        {
            Request(
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp)
                : session(NULL)
                , range(range)
                , seek_resp(seek_resp)
                , resp(resp)
            {
            }

            Session * session;
            SeekRange const & range;
            response_t seek_resp;  //async_open �ص� 
            response_t resp;  //async_open �ص� 
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_REQUEST_H_
