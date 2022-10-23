#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#	define BOOST_BEAST_USE_STD_STRING_VIEW
#endif // BOOST_BEAST_USE_STD_STRING_VIEW

#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "log.hpp"

namespace log
{

namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace net = boost::asio;		// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;	// from <boost/asio/ssl.hpp>

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
        // ssl::error::stream_truncated, also known as an SSL "short read",
        // indicates the peer closed the connection without performing the
        // required closing handshake (for example, Google does this to
        // improve performance). Generally this can be a security issue,
        // but if your communication protocol is self-terminated (as
        // it is with both HTTP) then you may simply ignore the lack of
        // close_notify.
        //
        // https://github.com/boostorg/beast/issues/38
        //
        // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
        //
        // When a short read would cut off the end of an HTTP message,
        // Beast returns the error beast::http::error::partial_message.
        // Therefore, if we see a short read here, it has occurred
        // after the message has been completed, so it is safe to ignore it.

        if(ec == ssl::error::stream_truncated)
                return;

        std::cerr << what << ": " << ec.message() << "\n";
}

} // namespace log
