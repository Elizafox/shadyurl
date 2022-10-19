#include <cerrno>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <system_error>

#include "mime.hpp"

namespace mime_type
{

std::string_view MimeTypeMap::find_extension(std::string extension_) const
{
	std::string extension{extension_};

	size_t pos;
	while((pos = extension.find(".")) != std::string::npos)
	{	
		extension.erase(0, pos + 1);
		auto mime_search = mimetypes.find(extension);
		if(mime_search != mimetypes.end())
			return mime_search->second;
	}

	return default_mimetype();
}

std::string_view MimeTypeMap::find_filename(std::string filename) const
{
	// We assume everything after the first dot is an extension
	size_t pos = filename.find(".");
	if(pos == std::string::npos)
		return default_mimetype();

	return find_extension(filename.substr(pos));
}

MimeTypeMap::MimeTypeMap()
{
	std::ifstream ifs{mimetypes_file()};
	if(!ifs.is_open())
		throw std::system_error(errno, std::generic_category());

	std::string line;
	while(std::getline(ifs, line))
	{
		std::string ext;
		std::string type;
		std::istringstream ss{line};

		ss >> ext >> type;
		mimetypes[ext] = type;
	}

	if(ifs.bad())
		throw std::system_error(errno, std::generic_category());
}

} // namespace mime_type
