#include <optional>
#include <cstdint>
#include <string_view>

#include <toml++/toml.h>

#include "server_state.hpp"
#include "mime.hpp"

namespace server_state
{

// This is a state class passed around to servers to keep the number of parameters low.
ServerState::ServerState(const toml::table& tbl, const mime_type::MimeTypeMap& mtm)
	: tbl_(tbl)
	, mtm_(mtm)
{
}

const toml::table& ServerState::get_config_table() const
{
	return tbl_;
}

const mime_type::MimeTypeMap& ServerState::get_mime_type_map() const
{
	return mtm_;
}

const std::uint32_t ServerState::get_config_threads() const
{
	std::optional<std::uint32_t> cfg_threads = tbl_["config"]["threads"].value<std::uint32_t>();
	if(!cfg_threads)
		return 2;

	return *cfg_threads;
}

std::string_view ServerState::get_config_address() const
{
	std::optional<std::string_view> cfg_address = tbl_["listen"]["ip"].value<std::string_view>();
	if(!cfg_address)
		return "0.0.0.0";

	return *cfg_address;
}

const std::uint16_t ServerState::get_config_port() const
{
	std::optional<std::uint16_t> cfg_port = tbl_["listen"]["port"].value<std::uint16_t>();
	if(!cfg_port)
		return 8080;
	
	return *cfg_port;
}

const std::uint16_t ServerState::get_config_port2() const
{
	std::optional<std::uint16_t> cfg_port2 = tbl_["listen"]["port2"].value<std::uint16_t>();
	if(!cfg_port2)
		return 0;

	return *cfg_port2;
}

std::string_view ServerState::get_config_doc_root() const
{
	std::optional<std::string_view> cfg_docroot = tbl_["config"]["docroot"].value<std::string_view>();
	if(!cfg_docroot)
		return ".";

	return *cfg_docroot;
}

std::string_view ServerState::get_config_hostname() const
{
	std::optional<std::string_view> cfg_hostname = tbl_["config"]["hostname"].value<std::string_view>();
	if(!cfg_hostname)
		return "z6a.info";

	return *cfg_hostname;
}

std::string_view ServerState::get_config_log_level() const
{
	std::optional<std::string_view> cfg_loglevel = tbl_["config"]["loglevel"].value<std::string_view>();
	if(!cfg_loglevel)
		return "info";

	return *cfg_loglevel;
}

} // namespace server_state
