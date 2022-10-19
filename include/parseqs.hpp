#ifndef PARSEQS_H
#define PARSEQS_H

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <map>

namespace parseqs
{

std::tuple<std::string, std::string> split_by_eq(std::string_view);
std::string unquote(std::string_view);
std::map<std::string, std::string> parse_qsl(std::string_view, std::size_t maxparams = 1000);

} // namespace parseqs

#endif // PARSEQS_H
