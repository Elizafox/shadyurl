//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP SSL server, coroutine
//
//------------------------------------------------------------------------------


#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#	define BOOST_BEAST_USE_STD_STRING_VIEW
#endif // BOOST_BEAST_USE_STD_STRING_VIEW

#include <syslog.h>
#include <stdarg.h>
#include <algorithm>
#include <cstdlib>
#include <cinttypes>
#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/config.hpp>

#include <toml++/toml.h>

#include "certificate.hpp"
#include "session.hpp"
#include "path.hpp"
#include "server_state.hpp"
#include "daemon.hpp"

namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;		// from <boost/beast/http.hpp>
namespace net = boost::asio;		// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;	// from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;	// from <boost/asio/ip/tcp.hpp>

int main(int argc, char* argv[])
{
	if(!daemonise::check_pid())
	{
		std::cerr << "Process already running" << std::endl;
		return EXIT_FAILURE;
	}

	// Open the logger
	openlog("urlshorten", LOG_PID | LOG_NDELAY, LOG_DAEMON);

	syslog(LOG_INFO, "Starting urlshorten");

	// Load config data
	toml::table tbl;
	try
	{
		tbl = toml::parse_file("config.toml");
	}
	catch(const toml::parse_error& err)
	{
		syslog(LOG_ALERT, "Parsing of config file failed: %s", err.what());
		return EXIT_FAILURE;
	}

	mime_type::MimeTypeMap mtm{"mimetypes.txt"};
	server_state::ServerState state{tbl, mtm};

	if(!logging::set_log_level(state.get_config_log_level()))
	{
		return EXIT_FAILURE;
	}

	auto const address = net::ip::make_address(state.get_config_address());
	auto const port = static_cast<unsigned short>(state.get_config_port());

	// The io_context is required for all I/O
	net::io_context ioc{static_cast<int>(state.get_config_threads())};

	// The SSL context is required, and holds certificates
	ssl::context ctx{ssl::context::tlsv12};

	// This holds the self-signed certificate used by the server
	if(!certificate::load_server_certificate(state, ctx))
	{
		return EXIT_FAILURE;
	}

	// Create and launch a listening port
	std::make_shared<session::listener>(
		ioc,
		ctx,
		tcp::endpoint{address, port},
		state)->run();

	if(state.get_config_port2())
	{
		auto const port2 = static_cast<unsigned short>(state.get_config_port2());
		std::make_shared<session::listener>(
			ioc,
			ctx,
			tcp::endpoint{address, port2},
			state)->run();
	}

	// Capture SIGINT and SIGTERM to perform a clean shutdown
	net::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait(
		[&](beast::error_code const&, int)
		{
			// Stop the `io_context`. This will cause `run()`
			// to return immediately, eventually destroying the
			// `io_context` and all of the sockets in it.
			ioc.stop();
		});

	// Ready to daemonise.
	ioc.notify_fork(net::io_context::fork_prepare);
	if(state.get_config_daemon())
	{
		bool is_daemon = daemonise::daemonise(daemonise::D_NO_CLOSE_FILES);
		if(is_daemon == false)
		{
			syslog(LOG_ALERT, "Could not daemonise: %s", strerror(errno));
			return EXIT_FAILURE;
		}
	}
	ioc.notify_fork(net::io_context::fork_child);

	if(!daemonise::write_pid())
	{
		syslog(LOG_ALERT, "Could not write PID file: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	// Drop privileges
	if(!daemonise::drop_privs(state.get_config_user(), state.get_config_group()))
	{
		syslog(LOG_ALERT, "Could not drop privileges: %s", strerror(errno));
		return EXIT_FAILURE;
	}

#if 0
	// Set rlimit
	if(!daemonise::set_rlimit())
	{
		syslog(LOG_ALERT, "Could not set rlimit: %s", strerror(errno));
		return EXIT_FAILURE;
	}
#endif

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(state.get_config_threads() - 1);
	for(auto i = state.get_config_threads() - 1; i > 0; --i)
		v.emplace_back(
		[&ioc]
		{
			ioc.run();
		});
	ioc.run();

	// (If we get here, it means we got a SIGINT or SIGTERM)

	daemonise::remove_pid();

	// Block until all the threads exit
	for(auto& t : v)
		t.join();

	return EXIT_SUCCESS;
}
