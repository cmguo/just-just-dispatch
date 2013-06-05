// Error.h

#ifndef _PPBOX_DISPATCH_ERROR_H_
#define _PPBOX_DISPATCH_ERROR_H_

namespace ppbox
{
    namespace dispatch
    {

        namespace error
        {
            enum errors
            {
                not_support = 1,
                invalid_url, 
                session_not_found, 
                session_not_open, 
                session_kick_out, 
                status_refuse, 
            };

            namespace detail {

                class dispatch_category
                    : public boost::system::error_category
                {
                public:
                    dispatch_category()
                    {
                        register_category(*this);
                    }

                    const char* name() const
                    {
                        return "dispatch";
                    }

                    std::string message(int value) const
                    {
                        switch (value) {
                            case not_support:
                                return "dispatch: not support";
                            case invalid_url:
                                return "dispatch: invalid url";
                            case session_not_found:
                                return "dispatch: session not found";
                            case session_not_open:
                                return "dispatch: session not open";
                            case session_kick_out:
                                return "dispatch: session kick out";
                            case status_refuse:
                                return "dispatch: status refuse";
                            default:
                                return "dispatcher: unknown error";
                        }
                    }
                };

            } // namespace detail

            inline const boost::system::error_category & get_category()
            {
                static detail::dispatch_category instance;
                return instance;
            }

            inline boost::system::error_code make_error_code(
                errors e)
            {
                return boost::system::error_code(
                    static_cast<int>(e), get_category());
            }

        } // namespace error

    } // namespace dispatch
} // namespace ppbox

namespace boost
{
    namespace system
    {

        template<>
        struct is_error_code_enum<ppbox::dispatch::error::errors>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
        using ppbox::dispatch::error::make_error_code;
#endif

    }
}

#endif // _PPBOX_DISPATCH_ERROR_H_
