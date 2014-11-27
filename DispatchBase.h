// DispatchBase.h

#ifndef _JUST_DISPATCH_DISPATCH_BASE_H_
#define _JUST_DISPATCH_DISPATCH_BASE_H_

#include <just/data/base/DataStat.h>

#include <just/avbase/Sample.h>
#include <just/avbase/StreamInfo.h>
#include <just/avbase/MediaInfo.h>
#include <just/avbase/StreamStatus.h>

#include <framework/string/Url.h>

#include <boost/function.hpp>

namespace util
{
    namespace stream
    {
        class Sink;
    }
}

namespace just
{
    namespace dispatch
    {

        using just::data::DataStat;

        using just::avbase::Sample;
        using just::avbase::StreamInfo;
        using just::avbase::MediaInfo;
        using just::avbase::StreamStatus;

        typedef boost::function<void (
            boost::system::error_code const &)> response_t;

        class DispatcherBase;

        static char const * const param_playlink = "playlink";
        static char const * const param_format = "format";
        static char const * const param_session = "session";

        struct SeekRange
        {
            enum TypeEnum
            {
                none, 
                byte, 
                time, 
            };

            SeekRange()
                : type(none)
                , beg(0)
                , end(0)
            {
            }

            SeekRange(
                TypeEnum type, 
                boost::uint64_t beg)
                : type(type)
                , beg(beg)
                , end(0)
            {
            }

            SeekRange(
                TypeEnum type, 
                boost::uint64_t beg, 
                boost::uint64_t end)
                : type(type)
                , beg(beg)
                , end(end)
            {
            }

            TypeEnum type;
            boost::uint64_t beg; 
            boost::uint64_t end; 
        };

    } // namespace dispatch
} // namespace just

#endif // _JUST_DISPATCH_DISPATCH_BASE_H_
