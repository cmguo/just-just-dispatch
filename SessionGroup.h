// SessionGroup.h

#ifndef _PPBOX_DISPATCH_SESSION_GROUP_H_
#define _PPBOX_DISPATCH_SESSION_GROUP_H_

#include "ppbox/dispatch/DispatchBase.h"

namespace ppbox
{
    namespace dispatch
    {

        class Session;
        struct Request;

        class Dispatcher;

        class SessionGroup
        {
        public:
            enum StatusEnum
            {
                waiting, 
                openning, 
                openned, 
                working, 
            };

        public:
            SessionGroup(
                framework::string::Url const & url, 
                Dispatcher & dispatcher);

        public:
            Request * request();

            void response(
                boost::system::error_code const & ec);

            void close(
                boost::system::error_code const & ec);

        public:
            Dispatcher & dispatcher() const
            {
                return dispatcher_;
            }

        public:
            framework::string::Url const & url() const
            {
                return url_;
            }

            bool busy() const
            {
                return status_ == openning 
                    || status_ == working;
            }

            Session * current() const
            {
                return current_;
            }

        public:
            bool accept(
                framework::string::Url const & url);

            /* 
             */
            void queue_session(
                Session * ses);

            /* 
                排队该会话，可能踢掉前面的会话中的请求
                当前影片必须已经打开，并且没有被另一个影片踢掉（正在取消中）
                不改变影片状态，可能会改变当前会话和等待会话
                返回值表示是否需要取消当前请求
             */
            bool active_session(
                Session * ses);

            /* 
                返回值表示是否需要取消当前请求
             */
            bool close_session(
                Session * ses);

            Session * find_session(
                size_t id) const;

        public:
            static Request * open_request;
            static Request * buffer_request;

            static Session * buffer_session;

        private:
            framework::string::Url url_;
            Dispatcher & dispatcher_;

        private:
            StatusEnum status_;
            std::vector<Session *> sessions_;
            Session * current_;
            Session * next_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SESSION_GROUP_H_
