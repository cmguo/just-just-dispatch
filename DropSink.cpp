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

        size_t DropSink::write(
            buffers_t const & buffers, 
            boost::system::error_code & ec)
        {
            ec.clear();
            return util::buffers::buffers_size(buffers);
        }


    } // namespace dispatch
} // namespace ppbox
