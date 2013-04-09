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
            Sink & sink)
        {
            if (index == (boost::uint32_t)-1) {
                default_sink_ = &sink;
            } else {
                if (sinks_.size() <= index)
                    sinks_.resize(index + 1, default_sink_);
                sinks_[index] = &sink;
            }
        }

        void SinkGroup::clear()
        {
            sinks_.clear();
            default_sink_ = &drop_sink_;
        }

        bool SinkGroup::write(
            Sample & sample, 
            boost::system::error_code & ec)
        {
            using boost::asio::buffer_size;

            size_t n = 0;
            if (sample.itrack < sinks_.size()) {
                n = sinks_[sample.itrack]->write(sample, ec);
            } else {
                n = default_sink_->write(sample, ec);
            }
            if (n == sample.size) {
                sample.data.clear();
                return true;
            } else {
                sample.size -= n;
                while (n >= buffer_size(sample.data.front())) {
                    n -= buffer_size(sample.data.front());
                    sample.data.pop_front();
                }
                if (n) {
                    sample.data.front() = sample.data.front() + n;
                }
                if (!ec) {
                    ec = boost::asio::error::would_block;
                }
                return false;
            }
        }

    } // namespace dispatch
} // namespace ppbox
