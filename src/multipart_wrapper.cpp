#include "multipart_wrapper.hpp"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <string_view>
#include <tuple>
#include <map>
#include <vector>
#include <variant>

namespace multipart_wrapper
{

typedef MultiPartData::MultiPartSection MultiPartSection;

MultiPartSection::header_type
MultiPartSection::parse_header_value(std::string_view header)
{
	std::vector<std::string> tokens;
	MultiPartSection::header_params_type data;

	boost::split(tokens, header, boost::is_any_of(";"));

	for(auto& token : tokens)
	{
		boost::trim_left(token);

		size_t pos;
		if((pos = token.find("=")) != std::string::npos)
		{
			std::string k = token.substr(0, pos);
			std::string v = token.substr(++pos);

			if(v[0] == '"')
			{
				std::stringstream ss;
				ss << v;
				ss >> std::quoted(v);
			}
			data[k] = v;
		}
		else
			data[token] = "";
	}

	if(data.empty())
		return std::string{};
	else
		return data;
}

MultiPartSection::MultiPartSection(const MultiPartSection::header_map_type& headers, std::string_view data)
	: headers_(headers)
	, data_(data)
{
}

MultiPartSection::header_map_type
MultiPartSection::get_headers() const
{
	return headers_;
}

std::string
MultiPartSection::get_data() const
{
	return data_;
}

MultiPartData::MultiPartData(std::string_view boundary)
	: boundary_(boundary)
{
	memset(&callbacks_, 0, sizeof(multipart_parser_settings));

	callbacks_.on_header_field = read_header_name;
	callbacks_.on_header_value = read_header_value;
	callbacks_.on_part_data = read_body;

	parser_ = multipart_parser_init(boundary_.data(), &callbacks_);
	multipart_parser_set_data(parser_, this);
}


MultiPartData::~MultiPartData()
{
	multipart_parser_free(parser_);
}

void
MultiPartData::ingest(std::string_view body)
{
	multipart_parser_execute(parser_, body.data(), body.size());
}

std::vector<MultiPartSection>
MultiPartData::get_data()
{
	return data_;
}

int
MultiPartData::read_header_name(multipart_parser* p, const char* at, size_t length)
{
	MultiPartData* mp = static_cast<MultiPartData*>(multipart_parser_get_data(p));
	mp->cur_header_name_.clear();
	mp->cur_header_name_.assign(at, length);
	return 0;
}

int
MultiPartData::read_header_value(multipart_parser* p, const char* at, size_t length)
{
	MultiPartData* mp = static_cast<MultiPartData*>(multipart_parser_get_data(p));
	std::string v{at, length};
	mp->cur_header_vals_[mp->cur_header_name_] = MultiPartSection::parse_header_value(v);
	return 0;
}

int
MultiPartData::read_body(multipart_parser* p, const char* at, size_t length)
{
	MultiPartData* mp = static_cast<MultiPartData*>(multipart_parser_get_data(p));
	mp->data_.push_back(MultiPartSection{mp->cur_header_vals_, std::string(at, length)});
	mp->cur_header_vals_.clear();
	return 0;
}

} // namespace multipart_wrapper
