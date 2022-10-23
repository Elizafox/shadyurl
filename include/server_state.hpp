#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <optional>
#include <cstdint>
#include <string_view>

#include <toml++/toml.h>

#include "mime.hpp"

namespace server_state
{

// This is a state class passed around to servers to keep the number of parameters low.
class ServerState
{
public:
	ServerState(const toml::table&, const mime_type::MimeTypeMap&);

	const toml::table& get_config_table() const;
	const mime_type::MimeTypeMap& get_mime_type_map() const;

	const std::uint32_t get_config_threads() const;
	const std::string_view get_config_address() const;
	const std::uint16_t get_config_port() const;
	const std::uint16_t get_config_port2() const;
	const std::string_view get_config_doc_root() const;
	const std::string_view get_config_hostname() const;
private:
	toml::table tbl_;
	mime_type::MimeTypeMap mtm_;
};

} // namespace server_state;

#endif // SERVER_STATE_H
