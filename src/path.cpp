#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "server_certificate.hpp"
#include "mime.hpp"
#include "path.hpp"

#include <algorithm>
#include <memory>
#include <string_view>
#include <string>


// Return a reasonable mime type based on the extension of a file.
std::string_view
get_mime_type(std::string_view path, std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
{
	auto const filename = [&path]
	{
		auto const pos = path.rfind("/");
		if(pos == std::string_view::npos)
			return path;
		return path.substr(pos);
	}();

	return mtm->find_filename(filename.data());
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(std::string_view base, std::string_view path)
{
	if(base.empty())
		return std::string(path);
	std::string result(base);
#ifdef BOOST_MSVC
	char constexpr path_separator = '\\';
	if(result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
	for(auto& c : result)
		if(c == '/')
			c = path_separator;
#else
	char constexpr path_separator = '/';
	if(result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
#endif
	return result;
}
