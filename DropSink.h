// DropSink.h

#ifndef _PPBOX_DISPATCH_DROP_SINK_H_
#define _PPBOX_DISPATCH_DROP_SINK_H_

#include "ppbox/dispatcher/DispatcherError.h"

#include <util/stream/Sink.h>

namespace ppbox
{
    namespace dispatch
    {

        class DropSink 
            : public util::stream::Sink
        {
        public:
            DropSink()
                : util::stream::Sink(*(boost::asio::io_service *)NULL)
            {
            }

            virtual ~DropSink() {}

        private:
            virtual std::size_t private_write_some(
                util::stream::StreamConstBuffers const & buffers,
                boost::system::error_code & ec)
            {
                ec.clear();
                return 0;
            }

        private:
            
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DROP_SINK_H_
