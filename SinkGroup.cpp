// SinkGroup.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SinkGroup.h"

namespace ppbox
{
    namespace dispatch
    {

        DropSink SinkGroup::drop_sink_;

        SinkGroup::SinkGroup()
            : default_sink_(&drop_sink_)
        {
        }

        void SinkGroup::setup(
            boost::uint32_t index, 
            util::stream::Sink & sink)
        {
            if (index == (boost::uint32_t)-1) {
                default_sink_ = &sink;
            } else {
                if (sinks_.size() <= index)
                    sinks_.resize(index + 1, default_sink_);
                sinks_[index] = &sink;
            }
        }

        size_t SinkGroup::write(
            boost::uint32_t index, 
            util::stream::Sink::buffers_t const & buffers, 
            boost::system::error_code & ec)
        {
            if (index < sinks_.size()) {
                return sinks_[index]->write_some(buffers, ec);
            } else {
                return default_sink_->write_some(buffers, ec);
            }
        }

    } // namespace dispatch
} // namespace ppbox
