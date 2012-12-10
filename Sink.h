// Sink.h

#ifndef _PPBOX_DISPATCH_SINK_SINK_H_
#define _PPBOX_DISPATCH_SINK_SINK_H_

#include "ppbox/dispatch/DispatchBase.h"

#include <util/stream/Sink.h>

namespace ppbox
{
    namespace dispatch
    {

        class Sink
        {
        public:
            virtual size_t write(
                Sample const & sample, 
                boost::system::error_code & ec) = 0;
        };

        class WrapSink
            : public Sink
        {
        public:
            WrapSink(
                util::stream::Sink & sink)
                : sink_(sink)
            {
            }

        public:
            util::stream::Sink & sink() const
            {
                return sink_;
            }

        public:
            virtual size_t write(
                Sample const & sample, 
                boost::system::error_code & ec)
            {
                return sink_.write_some(sample.data, ec);
            }

        private:
            util::stream::Sink & sink_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SINK_GROUP_H_
