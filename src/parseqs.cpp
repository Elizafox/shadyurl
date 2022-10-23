#include <boost/algorithm/string.hpp>

#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>

#include "parseqs.hpp"

namespace parseqs
{

static inline bool
is_hex(char c)
{
	return std::isdigit(c) ||
		(c >= 'a' && c <= 'f') ||
		(c >= 'A' && c <= 'F');
}

static inline char
from_hex(char c)
{
	return std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

std::tuple<std::string, std::string>
split_by_eq(std::string_view str)
{
	std::string k;
	std::string v;
	size_t pos = str.find("=");

	if(pos == 0)
	{
		// Only a value was found
		v = str;
	}
	else if(pos == std::string_view::npos)
	{
		// Only a key was found
		k = str;
	}
	else
	{
		k = str.substr(0, pos);
		v = str.substr(pos + 1);
	}

	return std::make_tuple(k, v);
}

std::string
unquote(std::string_view str)
{
	std::ostringstream unquoted;
	for(auto i = str.begin(), j = str.end(); i != j; i++)
	{
		std::string::value_type c = *i;

		switch(c)
		{
		case '%':
			if(i[1] && i[2])
			{
				// If this is hex, we convert it into a char
				// Otherwise, we silently discard it
				if(is_hex(i[1]) && is_hex(i[2]))
				{
					char h = from_hex(i[1]) << 4 | from_hex(i[2]);
					unquoted << h;
				}

				i += 2;
			}
			break;
		case '+':
			unquoted << ' ';
			break;
		default:
			unquoted << c;
			break;
		}
	}

	return unquoted.str();
}

std::map<std::string, std::string>
parse_qsl(std::string_view qs, std::size_t maxparams)
{
	std::vector<std::string> qsv;

	// To prevent DoS attacks, we limit the number of params we split out.
	if(static_cast<std::size_t>(std::count(qs.begin(), qs.end(), '&')) > maxparams)
	{
		throw std::length_error("maxparams exceeded");
	}

	boost::split(qsv, qs, boost::is_any_of("&"));

	std::map<std::string, std::string> qsm;
	for(auto& kv : qsv)
	{
		if(kv.empty())
			// Ignore empty values
			continue;

		std::tuple kvt = split_by_eq(kv);
		std::string k = unquote(std::get<0>(kvt));
		std::string v = unquote(std::get<1>(kvt));
		qsm[k] = v;
	}

	return qsm;
}

} // namespace parseqs
