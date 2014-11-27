// DispatchTask.h

#ifndef _JUST_DISPATCH_DISPATCH_TASK_H_
#define _JUST_DISPATCH_DISPATCH_TASK_H_

namespace just
{
    namespace dispatch
    {

        class DispathTaskBase
        {
        protected:
            DispathTaskBase(
                void (*perform)(DispathTaskBase &), 
                void (*destroy)(DispathTaskBase &))
                : perform_(perform)
                , destroy_(destroy)
            {
            }

        public:
            void perform()
            {
                perform_(*this);
                destroy_(*this);
            }

        private:
            void (*perform_)(DispathTaskBase &);
            void (*destroy_)(DispathTaskBase &);
        };

        template <typename Task>
        class DispathTask
            : public DispathTaskBase
        {
        public:
            DispathTask(
                Task const & task)
                : DispathTaskBase(perform, destroy)
                , task_(task)
            {
            }

        private:
            static void perform(
                DispathTaskBase & task)
            {
                DispathTask & self = static_cast<DispathTask &>(task);
                self.task_();
            }

            static void destroy(
                DispathTaskBase & task)
            {
                DispathTask & self = static_cast<DispathTask &>(task);
                delete &self;
            }

        private:
            Task task_;
        };

        template <typename Task>
        DispathTaskBase * make_task(
            Task const & task)
        {
            return new DispathTask<Task>(task);
        }

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_DISPATCH_TASK_H_
