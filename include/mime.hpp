#ifndef MIME_H
#define MIME_H

#include <cerrno>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <system_error>


namespace mime_type
{

class MimeTypeMap
{
private:
	std::unordered_map<std::string, std::string> mimetypes;
	std::string mimetypes_file_;
	std::string default_mimetype_;
public:
	MimeTypeMap(std::string_view mimetypes_file, std::string_view default_mimetype);

	MimeTypeMap(std::string_view mimetypes_file)
		: MimeTypeMap(mimetypes_file, "application/octet-stream")
	{
	}

	MimeTypeMap()
		: MimeTypeMap("mimetypes.txt", "application/octet-stream")
	{
	}

	std::string_view find_extension(std::string) const;
	std::string_view find_filename(std::string) const;
};

} // namespace mime_type

#endif // MIME_H
