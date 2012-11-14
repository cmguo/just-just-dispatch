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
                bufferring, 
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
                boost::system::error_code const & ec, 
                std::vector<Session *> & orphans);

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
                    || status_ == working
                    || status_ == bufferring;
            }

            bool ready() const
            {
                return status_ == openned 
                    || status_ == working
                    || status_ == bufferring;
            }

            bool empty() const
            {
                return current_ == NULL;
            }

            Session * first() const
            {
                return first_;
            }

            Session * current() const
            {
                return current_;
            }

            Session * next() const
            {
                return current_;
            }

        public:
            bool accept(
                framework::string::Url const & url);

            /* 
                排队该会话，可能踢掉前面的会话中的请求
                当前影片没有被另一个影片踢掉（正在取消中）
                不改变影片状态，可能会改变当前会话和等待会话
                下列情形返回值为true
                1、需要取消当前会话
                2、没有当前会话，需要处理后续请求
             */
            bool queue_session(
                Session * ses);

            bool active_session(
                Session * ses);

            /* 
                下列情形返回值为true
                1、当前会话被关闭
             */
            bool close_session(
                Session * ses);

            void replace(
                Session * ses_from, 
                Session * ses_to);

            Session * find_session(
                size_t id, 
                Session *& main_ses) const;

        public:
            static Request * open_request;
            static Request * switch_request;
            static Request * buffer_request;
            static Request * cancel_request;
            static Request * delete_request;

            static Session * delete_session;

        private:
            framework::string::Url url_;
            TaskDispatcher & dispatcher_;

        private:
            StatusEnum status_;
            Session * first_;
            Session * current_;
            Session * next_;
            bool buffer_finish_;
            std::vector<Session *> kick_outs_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SESSION_GROUP_H_
