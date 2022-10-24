#ifndef PATH_H
#define PATH_H

#include <string_view>
#include <string>
#include <memory>

#include "mime.hpp"

namespace pathutil
{

std::string_view get_mime_type(std::string_view, const mime_type::MimeTypeMap&);
std::string path_cat(std::string_view, std::string_view);

} // namespace pathutil

#endif // PATH_H
