#ifndef MIME_H
#define MIME_H

#include <cerrno>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <system_error>


namespace mime_type
{

class MimeTypeMap
{
private:
	std::unordered_map<std::string, std::string> mimetypes;

protected:
	const char* default_mimetype() const { return "application/octet-stream"; }
	const char* mimetypes_file() const { return "mimetypes.txt"; }
public:
	std::string_view find_extension(std::string) const;
	std::string_view find_filename(std::string) const;
	MimeTypeMap();
};

} // namespace mime_type

#endif // MIME_H
