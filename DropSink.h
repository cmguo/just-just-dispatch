// DropSink.h

#ifndef _PPBOX_DISPATCH_DROP_SINK_H_
#define _PPBOX_DISPATCH_DROP_SINK_H_

#include "ppbox/dispatch/Sink.h"

namespace ppbox
{
    namespace dispatch
    {

        class DropSink 
            : public ppbox::dispatch::Sink
        {
        public:
            DropSink();

        private:
            virtual size_t write(
                Sample const & sample, 
                boost::system::error_code & ec);
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_DROP_SINK_H_
