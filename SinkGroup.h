// SinkGroup.h

#ifndef _PPBOX_DISPATCH_SINK_GROUP_H_
#define _PPBOX_DISPATCH_SINK_GROUP_H_

#include "ppbox/dispatch/DropSink.h"

namespace ppbox
{
    namespace dispatch
    {

        class SinkGroup
        {
        public:
            SinkGroup();

        public:
            void setup(
                boost::uint32_t index, 
                Sink & sink);

            void clear();

            bool write(
                Sample & sample, 
                boost::system::error_code & ec);

        public:
            size_t sink_count() const
            {
                return sinks_.size();
            }

            bool sink_is_set(
                size_t index) const
            {
                return sinks_[index] != &drop_sink_;
            }

            bool default_sink_is_set() const
            {
                return default_sink_ != &drop_sink_;
            }

        private:
            static DropSink drop_sink_;

        private:
            std::vector<Sink *> sinks_;
            Sink * default_sink_;
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_DISPATCH_SINK_GROUP_H_
