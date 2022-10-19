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


#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "server_certificate.hpp"
#include "http_server.hpp"
#include "path.hpp"
#include "toml.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <cinttypes>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;		// from <boost/beast/http.hpp>
namespace net = boost::asio;		// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;	// from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;	// from <boost/asio/ip/tcp.hpp>

int main(int argc, char* argv[])
{
	// Load config data
	toml::table tbl;
	try
	{
		tbl = toml::parse_file("config.toml");
	}
	catch (const toml::parse_error& err)
	{
		std::cerr << "Parsing failed:\n" << err << "\n";
		return 1;
	}

	std::optional<std::uint32_t> cfg_threads = tbl["config"]["threads"].value<std::uint32_t>();

	std::optional<std::string_view> cfg_address = tbl["listen"]["ip"].value<std::string_view>();
	if(!cfg_address)
		*cfg_address = "0.0.0.0";

	std::optional<std::uint16_t> cfg_port = tbl["listen"]["port"].value<std::uint16_t>();
	if(!cfg_port)
		*cfg_port = 8480;

	std::optional<std::uint16_t> cfg_port2 = tbl["listen"]["port2"].value<std::uint16_t>();

	std::optional<std::string_view> cfg_docroot = tbl["config"]["docroot"].value<std::string_view>();
	if(!cfg_docroot)
		*cfg_docroot = ".";

	if(!cfg_threads)
		*cfg_threads = 1;

	// Load MIME types
	auto const mtm = std::make_shared<mime_type::MimeTypeMap const>(mime_type::MimeTypeMap{});;

	auto const address = net::ip::make_address(*cfg_address);
	auto const port = static_cast<unsigned short>(*cfg_port);
	auto const doc_root = std::make_shared<std::string>(*cfg_docroot);
	auto const threads = std::max<int>(1, *cfg_threads);

	// The io_context is required for all I/O
	net::io_context ioc{threads};

	// The SSL context is required, and holds certificates
	ssl::context ctx{ssl::context::tlsv12};

	// This holds the self-signed certificate used by the server
	load_server_certificate(ctx);

	// Create and launch a listening port
	std::make_shared<http_server::listener>(
		ioc,
		ctx,
		tcp::endpoint{address, port},
		doc_root,
		mtm)->run();

	if(cfg_port2)
	{
		auto const port2 = static_cast<unsigned short>(*cfg_port2);
		std::make_shared<http_server::listener>(
			ioc,
			ctx,
			tcp::endpoint{address, port2},
			doc_root,
			mtm)->run();
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

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for(auto i = threads - 1; i > 0; --i)
		v.emplace_back(
		[&ioc]
		{
			ioc.run();
		});
	ioc.run();

	// (If we get here, it means we got a SIGINT or SIGTERM)

	// Block until all the threads exit
	for(auto& t : v)
		t.join();

	return EXIT_SUCCESS;
}
