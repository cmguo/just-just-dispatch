// DropSink.h

#ifndef _JUST_DISPATCH_DROP_SINK_H_
#define _JUST_DISPATCH_DROP_SINK_H_

#include <util/stream/Sink.h>

namespace just
{
    namespace dispatch
    {

        class DropSink 
            : public util::stream::Sink
        {
        public:
            DropSink();

        private:
            virtual size_t private_write_some(
                buffers_t const & buffers, 
                boost::system::error_code & ec);
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_DROP_SINK_H_
