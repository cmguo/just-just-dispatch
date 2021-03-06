// Request.h

#ifndef _JUST_DISPATCH_REQUEST_H_
#define _JUST_DISPATCH_REQUEST_H_

#include "just/dispatch/DispatchBase.h"

namespace just
{
    namespace dispatch
    {

        class Session;

        struct Request
        {
            Request(
                Session * session, 
                SeekRange const & range, 
                response_t const & seek_resp, 
                response_t const & resp)
                : session(session)
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
            response_t seek_resp;  //async_open �ص� 
            response_t resp;  //async_open �ص� 
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_REQUEST_H_
