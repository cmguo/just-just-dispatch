// DropSink.h

#include "just/dispatch/Common.h"
#include "just/dispatch/DropSink.h"

#include <util/buffers/BuffersSize.h>

namespace just
{
    namespace dispatch
    {

        DropSink::DropSink()
            : util::stream::Sink(*(boost::asio::io_service *)NULL)
        {
        }

        size_t DropSink::private_write_some(
            buffers_t const & buffers, 
            boost::system::error_code & ec)
        {
            ec.clear();
            return util::buffers::buffers_size(buffers);
        }


    } // namespace dispatch
} // namespace just
