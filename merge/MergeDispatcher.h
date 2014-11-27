// MergeDispatcher.h

#ifndef _JUST_DISPATCH_MERGE_MERGE_DISPATCHER_H_
#define _JUST_DISPATCH_MERGE_MERGE_DISPATCHER_H_

#include "just/dispatch/TaskDispatcher.h"

namespace just
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
                boost::asio::io_service & io_svc);

            virtual ~MergeDispatcher();

        public:
            virtual bool get_media_info(
                MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_stream_info(
                std::vector<StreamInfo> & streams, 
                boost::system::error_code & ec);

            virtual bool get_data_stat(
                DataStat & stat, 
                boost::system::error_code & ec);

        public:
            virtual bool seek(
                SeekRange & range, 
                boost::system::error_code & ec);

            virtual bool read(
                Sample & sample, 
                boost::system::error_code & ec);

            virtual bool free(
                Sample & sample, 
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
                StreamStatus & status, 
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
            just::merge::MergeModule & merge_module_;

            size_t  merge_close_token_;
            just::merge::MergerBase * merger_;

            std::string format_;

            bool cancel_token_;
            bool pause_token_;
        };

        JUST_REGISTER_DISPATCHER(0, MergeDispatcher);

    } // namespace dispatcher
} // namespace just

#endif // _JUST_DISPATCH_MERGE_MERGE_DISPATCHER_H_
