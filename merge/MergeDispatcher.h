// MergeDispatcher.h

#ifndef _PPBOX_DISPATCH_MERGE_MERGE_DISPATCHER_H_
#define _PPBOX_DISPATCH_MERGE_MERGE_DISPATCHER_H_

#include "ppbox/dispatch/TaskDispatcher.h"

namespace ppbox
{
    namespace merge
    {
        class MergerBase;
        class MergeModule;
    }

    namespace dispatch
    {

        class MergeDispatcher 
            : public TaskDispatcher
        {
        public:
            MergeDispatcher(
                boost::asio::io_service & io_svc, 
                boost::asio::io_service & dispatch_io_svc);

            virtual ~MergeDispatcher();

        public:
            virtual bool get_media_info(
                MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_stream_info(
                std::vector<StreamInfo> & streams, 
                boost::system::error_code & ec);

        public:
            virtual bool accept(
                framework::string::Url const & url);

            virtual bool switch_to(
                framework::string::Url const & url, 
                boost::system::error_code & ec);

        private:
            virtual void start_open(
                framework::string::Url const & url);

            virtual void do_setup(
                boost::uint32_t index,      // Á÷±àºÅ
                boost::system::error_code & ec); 

            virtual void start_play(
                SeekRange const & range, 
                response_t const & seek_resp);

            virtual void start_buffer();

            virtual void cancel_open(
                boost::system::error_code & ec);

            virtual void do_get_stream_status(
                StreamStatus & info, 
                boost::system::error_code & ec);

        public:
            virtual void do_close(
                boost::system::error_code & ec);

       private:
            void handle_open(
                boost::system::error_code const & ec);

            void handle_play(
                boost::system::error_code const & ec);

            void handle_buffer(
                boost::system::error_code const & ec);

        private:
            ppbox::merge::MergeModule & merge_module_;

            size_t  merge_close_token_;
            ppbox::merge::MergerBase * merger_;

            std::string format_;

            bool cancel_token_;
            bool pause_token_;
        };

        PPBOX_REGISTER_DISPATCHER(0, MergeDispatcher);

    } // namespace dispatcher
} // namespace ppbox

#endif // _PPBOX_DISPATCH_MERGE_MERGE_DISPATCHER_H_
