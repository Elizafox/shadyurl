#ifndef MULTIPART_WRAPPER_H
#define MULTIPART_WRAPPER_H

#include "multipart_parser.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <map>
#include <vector>
#include <variant>

namespace multipart_wrapper
{

class MultiPartData
{
public:
	class MultiPartSection
	{
	public:
		using header_params_type = std::map<std::string, std::string>;
		using header_type = std::variant<std::string, header_params_type>;
		using header_map_type = std::map<std::string, header_type>;

		MultiPartSection(const header_map_type&, std::string_view);
		header_map_type get_headers() const;
		std::string get_data() const;
		static header_type parse_header_value(std::string_view);
	private:
		friend class MultiPartData;
		header_map_type headers_;
		std::string data_;
	};

	MultiPartData(std::string_view);

	// This object cannot safely be copied or moved
	MultiPartData(const MultiPartData&) = delete;
	MultiPartData(MultiPartData&&) = delete;
	MultiPartData& operator=(const MultiPartData&) = delete;
	MultiPartData& operator=(const MultiPartData&&) = delete;

	~MultiPartData();

	void ingest(std::string_view);
	std::vector<MultiPartSection> get_data();

private:
	static int read_header_name(multipart_parser*, const char*, size_t);
	static int read_header_value(multipart_parser*, const char*, size_t);
	static int read_body(multipart_parser*, const char*, size_t);

	std::string_view boundary_;
	std::vector<MultiPartSection> data_;
	multipart_parser* parser_;
	multipart_parser_settings callbacks_;

	// Temporaries
	MultiPartSection::header_map_type cur_header_vals_;
	std::string cur_header_name_;
};

} // namespace multipart_wrapper

#endif // MULTIPART_WRAPPER_H
