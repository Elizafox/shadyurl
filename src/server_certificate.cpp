#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>

#include "server_certificate.hpp"

/* Load a signed certificate into the ssl context, and configure
 * the context for use with a server.
 */
void
load_server_certificate(boost::asio::ssl::context& ctx)
{
	using namespace boost::asio;

	ctx.set_password_callback(
		[](std::size_t,
			ssl::context_base::password_purpose)
		{
			return "test";
		});

	ctx.set_options(
		ssl::context::default_workarounds |
		ssl::context::no_sslv2 |
		ssl::context::single_dh_use);

	ctx.use_certificate_chain_file("cert.pem");
	ctx.use_private_key_file("key.pem", ssl::context::file_format::pem);
	ctx.use_tmp_dh_file("dh.pem");
}
