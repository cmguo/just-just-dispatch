// Session.h

#ifndef _PPBOX_DISPATCH_SESSION_H_
#define _PPBOX_DISPATCH_SESSION_H_

#include "ppbox/dispatch/DispatchBase.h"
#include "ppbox/dispatch/Request.h"
#include "ppbox/dispatch/SinkGroup.h"

#include <framework/string/Url.h>

namespace ppbox
{
    namespace dispatch
    {

        class Session
        {
        public:
            Session(
                framework::string::Url const & url, 
                response_t const & resp);

        public:
            Request * request();

            void response(
                boost::system::error_code const & ec);

            void cancel();

            void mark_close();

            void close(
                boost::system::error_code const & ec);

        public:
            // 每个会话都有一个id，id为0表示外面已经close
            boost::uint32_t id() const
            {
                return id_;
            }

            framework::string::Url const & url() const
            {
                return url_;
            }

            SinkGroup & sink_group()
            {
                return sink_group_;
            }

            bool closed() const
            {
                return id_ == 0;
            }

            bool opened() const
            {
                return resp_.empty();
            }

        public:
            void queue_request(
                Request const & r);

        private:
            void response_all(
                boost::system::error_code const & ec);

        private:
            boost::uint32_t id_; 
            framework::string::Url url_;
            response_t resp_;  //async_open 回调 
            SinkGroup sink_group_;
            std::deque<Request> play_reqs_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SESSION_H_
