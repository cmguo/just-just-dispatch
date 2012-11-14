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

            struct find_by_session
            {
                find_by_session(
                    Session * ses)
                    : ses_(ses)
                {
                }

                bool operator()(
                    Request const & req)
                {
                    return req.session == ses_;
                }

            private:
                Session * ses_;
            };

            Session * session;
            SeekRange range;
            response_t seek_resp;  //async_open 回调 
            response_t resp;  //async_open 回调 
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_REQUEST_H_
