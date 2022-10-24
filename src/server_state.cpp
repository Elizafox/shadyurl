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

std::uint32_t ServerState::get_config_threads() const
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

std::uint16_t ServerState::get_config_port() const
{
	std::optional<std::uint16_t> cfg_port = tbl_["listen"]["port"].value<std::uint16_t>();
	if(!cfg_port)
		return 8080;

	return *cfg_port;
}

std::uint16_t ServerState::get_config_port2() const
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

bool ServerState::get_config_daemon() const
{
	std::optional<bool> cfg_daemon = tbl_["config"]["daemon"].value<bool>();
	if(!cfg_daemon)
		return false;

	return *cfg_daemon;
}

std::string_view ServerState::get_config_cert_file() const
{
	std::optional<std::string_view> cfg_certfile = tbl_["config"]["certfile"].value<std::string_view>();
	if(!cfg_certfile)
		return "cert.pem";

	return *cfg_certfile;
}

std::string_view ServerState::get_config_key_file() const
{
	std::optional<std::string_view> cfg_keyfile = tbl_["config"]["keyfile"].value<std::string_view>();
	if(!cfg_keyfile)
		return "key.pem";

	return *cfg_keyfile;
}

std::string_view ServerState::get_config_dh_file() const
{
	std::optional<std::string_view> cfg_dhfile = tbl_["config"]["dhfile"].value<std::string_view>();
	if(!cfg_dhfile)
		return "dh.pem";

	return *cfg_dhfile;
}

std::string_view ServerState::get_config_user() const
{
	std::optional<std::string_view> cfg_user = tbl_["config"]["user"].value<std::string_view>();
	if(!cfg_user)
		return "";

	return *cfg_user;
}

std::string_view ServerState::get_config_group() const
{
	std::optional<std::string_view> cfg_group = tbl_["config"]["group"].value<std::string_view>();
	if(!cfg_group)
		return "";

	return *cfg_group;
}

std::string_view ServerState::get_config_db_path() const
{
	std::optional<std::string_view> cfg_dbpath = tbl_["config"]["dbpath"].value<std::string_view>();
	if(!cfg_dbpath)
		return "urls.db";

	return *cfg_dbpath;
}

} // namespace server_state
