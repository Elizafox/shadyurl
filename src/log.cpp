#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#	define BOOST_BEAST_USE_STD_STRING_VIEW
#endif // BOOST_BEAST_USE_STD_STRING_VIEW

#include <syslog.h>
#include <stdarg.h>
#include <iostream>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "log.hpp"

namespace logging
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

	syslog(LOG_INFO, "%s: %s", what, ec.message().c_str());
}

bool
set_log_level(std::string_view level)
{
	int log_level;
	if(boost::iequals(level, "debug"))
	{
		log_level = LOG_DEBUG;
	}
	else if(boost::iequals(level, "info"))
	{
		log_level = LOG_INFO;
	}
	else if(boost::iequals(level, "notice"))
	{
		log_level = LOG_NOTICE;
	}
	else if(boost::iequals(level, "warning"))
	{
		log_level = LOG_WARNING;
	}
	else if(boost::iequals(level, "error"))
	{
		log_level = LOG_ERR;
	}
	else if(boost::iequals(level, "critical"))
	{
		log_level = LOG_CRIT;
	}
	else if(boost::iequals(level, "alert"))
	{
		log_level = LOG_ALERT;
	}
	else
	{
		syslog(LOG_ALERT, "Unknown log level %s", level.data());
		return false;
	}

	setlogmask(LOG_UPTO(log_level));
	return true;
}

} // namespace log
