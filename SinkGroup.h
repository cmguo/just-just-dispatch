// SinkGroup.h

#ifndef _JUST_DISPATCH_SINK_GROUP_H_
#define _JUST_DISPATCH_SINK_GROUP_H_

#include "just/dispatch/DispatchBase.h"
#include "just/dispatch/DropSink.h"

namespace just
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
                util::stream::Sink & sink);

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
            std::vector<util::stream::Sink *> sinks_;
            util::stream::Sink * default_sink_;
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_SINK_GROUP_H_
