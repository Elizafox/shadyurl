#include <syslog.h>
#include <stdarg.h>
#include <cstdio>
#include <cerrno>
#include <stdexcept>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>

#include "certificate.hpp"
#include "server_state.hpp"

namespace certificate
{

/* Load a signed certificate into the ssl context, and configure
 * the context for use with a server.
 */
bool
load_server_certificate(const server_state::ServerState& state, boost::asio::ssl::context& ctx)
{
	using namespace boost::asio;

	ctx.set_options(
		ssl::context::default_workarounds |
		ssl::context::no_sslv2 |
		ssl::context::single_dh_use);

	try
	{
		ctx.use_certificate_chain_file(state.get_config_cert_file().data());
	}
	catch(std::exception& e)
	{
		syslog(LOG_ALERT, "Could not load certificate file: %s", e.what());
		return false;
	}

	try
	{
		ctx.use_private_key_file(state.get_config_key_file().data(), ssl::context::file_format::pem);
	}
	catch(std::exception& e)
	{
		syslog(LOG_ALERT, "Could not load private key file: %s", e.what());
		return false;
	}

	try
	{
		ctx.use_tmp_dh_file(state.get_config_dh_file().data());
	}
	catch(std::exception& e)
	{
		syslog(LOG_ALERT, "Could not load DH parameter file: %s", e.what());
		return false;
	}

	return true;
}

} // namespace certificate
