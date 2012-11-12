// DropSink.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/DropSink.h"

#include <util/buffers/BuffersSize.h>

namespace ppbox
{
    namespace dispatch
    {

        DropSink::DropSink()
            : util::stream::Sink(*(boost::asio::io_service *)NULL)
        {
        }

        std::size_t DropSink::private_write_some(
            util::stream::StreamConstBuffers const & buffers,
            boost::system::error_code & ec)
        {
            ec.clear();
            return util::buffers::buffers_size(buffers);
        }


    } // namespace dispatch
} // namespace ppbox