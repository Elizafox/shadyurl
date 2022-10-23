#ifndef LOG_H
#define LOG_H

#include <boost/beast.hpp>

namespace log
{

namespace beast = boost::beast;		// from <boost/beast.hpp>

void fail(beast::error_code, char const*);

} // namespace log

#endif // LOG_H
