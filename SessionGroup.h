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
                �ŶӸûỰ�������ߵ�ǰ��ĻỰ�е�����
                ��ǰӰƬ�����Ѿ��򿪣�����û�б���һ��ӰƬ�ߵ�������ȡ���У�
                ���ı�ӰƬ״̬�����ܻ�ı䵱ǰ�Ự�͵ȴ��Ự
                �������η���ֵΪtrue
                1����Ҫȡ����ǰ�Ự
                2��û�е�ǰ�Ự����Ҫ�����������
             */
            bool active_session(
                Session * ses);

            /* 
                �������η���ֵΪtrue
                1����ǰ�Ự���ر�
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
