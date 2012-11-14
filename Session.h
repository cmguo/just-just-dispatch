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
                boost::asio::io_service & io_svc, 
                framework::string::Url const & url, 
                response_t const & resp);

            Session(
                boost::asio::io_service & io_svc, 
                framework::string::Url const & url); // 虚拟会话

        public:
            Request * request();

            void response(
                boost::system::error_code const & ec);

            void cancel(
                boost::system::error_code const & ec);

            void mark_close();

            void close(
                std::vector<Session *> & orphans);

        public:
            struct find_by_id
            {
                find_by_id(
                    size_t id)
                    : id_(id)
                {
                }

                bool operator()(
                    Session * s)
                {
                    return s->id() == id_;
                }

            private:
                size_t id_;
            };

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
            void queue_sub(
                Session * ses);

            bool close_sub(
                Session * ses);

            Session * find_sub(
                boost::uint32_t id);

            Session * current_sub() const
            {
                return current_;
            };

        public:
            void queue_request(
                Request const & r);

        private:
            void response_all(
                boost::system::error_code const & ec);

            void response_sub_open(
                boost::system::error_code const & ec);

            void cancel_sub_request(
                Session * ses, 
                boost::system::error_code const & ec);

        public:
            static Request * setup_request;

        private:
            boost::uint32_t id_;
            boost::asio::io_service & io_svc_;
            framework::string::Url url_;
            response_t resp_;  //async_open 回调 
            SinkGroup sink_group_;
            std::deque<Request> play_reqs_;
            Session * current_;
            bool playing_;
            std::vector<Session *> sub_sessions_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SESSION_H_
