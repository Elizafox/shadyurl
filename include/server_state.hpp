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

	std::uint32_t get_config_threads() const;
	std::string_view get_config_address() const;
	std::uint16_t get_config_port() const;
	std::uint16_t get_config_port2() const;
	std::string_view get_config_doc_root() const;
	std::string_view get_config_hostname() const;
	std::string_view get_config_log_level() const;
	bool get_config_daemon() const;
	std::string_view get_config_cert_file() const;
	std::string_view get_config_key_file() const;
	std::string_view get_config_dh_file() const;
private:
	toml::table tbl_;
	mime_type::MimeTypeMap mtm_;
};

} // namespace server_state;

#endif // SERVER_STATE_H
