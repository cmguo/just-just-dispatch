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

        class TaskDispatcher;

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
                TaskDispatcher & dispatcher);

        public:
            Request * request();

            void response(
                boost::system::error_code const & ec);

            void close(
                boost::system::error_code const & ec);

        public:
            TaskDispatcher & dispatcher() const
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

            bool ready() const
            {
                return status_ == openned 
                    || status_ == working;
            }

            bool empty() const
            {
                return sessions_.empty();
            }

            Session * first() const
            {
                return first_;
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
                下列情形返回值为true
                1、需要取消当前会话
                2、没有当前会话，需要处理后续请求
             */
            bool active_session(
                Session * ses);

            /* 
                下列情形返回值为true
                1、当前会话被关闭
             */
            bool close_session(
                Session * ses);

            Session * find_session(
                size_t id) const;

        public:
            static Request * open_request;
            static Request * buffer_request;
            static Request * delete_request;

            static Session * buffer_session;
            static Session * delete_session;

        private:
            framework::string::Url url_;
            TaskDispatcher & dispatcher_;

        private:
            StatusEnum status_;
            std::vector<Session *> sessions_;
            Session * first_;
            Session * current_;
            Session * next_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SESSION_GROUP_H_
