// DropSink.h

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/DropSink.h"

namespace ppbox
{
    namespace dispatch
    {

        DropSink::DropSink()
        {
        }

        size_t DropSink::write(
            Sample const & sample, 
            boost::system::error_code & ec)
        {
            ec.clear();
            return sample.size;
        }


    } // namespace dispatch
} // namespace ppbox
