// SharedDispatcher.h

#ifndef _JUST_DISPATCH_SHARED_DISPATCHER_H_
#define _JUST_DISPATCH_SHARED_DISPATCHER_H_

#include "just/dispatch/DispatcherBase.h"

namespace just
{
    namespace dispatch
    {

        class SessionManager;

        class SharedDispatcher
            : public DispatcherBase
        {
        public:
            SharedDispatcher(
                SessionManager & manager);

            virtual ~SharedDispatcher();

        public:
            virtual void async_open(
                framework::string::Url const & url, 
                response_t const & resp);

            virtual bool setup(
                boost::uint32_t index,      // �����
                util::stream::Sink & sink, 
                boost::system::error_code & ec); 

            virtual void async_play(
                SeekRange const & range, 
                response_t const & seek_resp,
                response_t const & resp);

            virtual bool seek(
                SeekRange & range, 
                boost::system::error_code & ec);

            virtual bool read(
                Sample & sample, 
                boost::system::error_code & ec);

            virtual bool free(
                Sample & sample, 
                boost::system::error_code & ec);

            virtual bool pause(
                boost::system::error_code & ec);

            virtual bool resume(
                boost::system::error_code & ec);

            virtual bool get_media_info(
                MediaInfo & info, 
                boost::system::error_code & ec);

            virtual bool get_stream_info(
                std::vector<StreamInfo> & stream, 
                boost::system::error_code & ec);

            virtual bool get_stream_status(
                StreamStatus & status, 
                boost::system::error_code & ec);

            virtual bool get_data_stat(
                DataStat & stat, 
                boost::system::error_code & ec);

            virtual bool cancel(
                boost::system::error_code & ec);

            virtual bool close(
                boost::system::error_code & ec);

        private:
            SessionManager & manager_;
            boost::uint32_t session_id_;
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_SHARED_DISPATCHER_H_
