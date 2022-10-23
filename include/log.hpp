#ifndef LOG_H
#define LOG_H

#include <string_view>

#include <boost/beast.hpp>

namespace log
{

namespace beast = boost::beast;		// from <boost/beast.hpp>

void fail(beast::error_code, char const*);

bool set_log_level(std::string_view);

} // namespace log

#endif // LOG_H
