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


#include "server_certificate.hpp"
#include "session.hpp"
#include "path.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
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


namespace session
{

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
	// ssl::error::stream_truncated, also known as an SSL "short read",
	// indicates the peer closed the connection without performing the
	// required closing handshake (for example, Google does this to
	// improve performance). Generally this can be a security issue,
	// but if your communication protocol is self-terminated (as
	// it is with both HTTP) then you may simply ignore the lack of
	// close_notify.
	//
	// https://github.com/boostorg/beast/issues/38
	//
	// https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
	//
	// When a short read would cut off the end of an HTTP message,
	// Beast returns the error beast::http::error::partial_message.
	// Therefore, if we see a short read here, it has occurred
	// after the message has been completed, so it is safe to ignore it.

	if(ec == net::ssl::error::stream_truncated)
		return;

	std::cerr << what << ": " << ec.message() << "\n";
}

//------------------------------------------------------------------------------

// Start the session
void plain_http_session::run()
{
	this->do_read();
}

// Called by the base class
beast::tcp_stream& plain_http_session::stream()
{
	return stream_;
}

// Called by the base class
beast::tcp_stream plain_http_session::release_stream()
{
	return std::move(stream_);
}

// Called by the base class
void plain_http_session::do_eof()
{
	// Send a TCP shutdown
	beast::error_code ec;
	stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

	// At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------

// Start the session
void ssl_http_session::run()
{
	// Set the timeout.
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

	// Perform the SSL handshake
	// Note, this is the buffered version of the handshake.
	stream_.async_handshake(
		ssl::stream_base::server,
		buffer_.data(),
		beast::bind_front_handler(
			&ssl_http_session::on_handshake,
			shared_from_this()));
}

// Called by the base class
beast::ssl_stream<beast::tcp_stream>& ssl_http_session::stream()
{
	return stream_;
}

// Called by the base class
beast::ssl_stream<beast::tcp_stream> ssl_http_session::release_stream()
{
	return std::move(stream_);
}

// Called by the base class
void ssl_http_session::do_eof()
{
	// Set the timeout.
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

	// Perform the SSL shutdown
	stream_.async_shutdown(
		beast::bind_front_handler(
			&ssl_http_session::on_shutdown,
			shared_from_this()));
}

void ssl_http_session::on_handshake(beast::error_code ec, std::size_t bytes_used)
{
	if(ec)
		return fail(ec, "handshake");

	// Consume the portion of the buffer used by the handshake
	buffer_.consume(bytes_used);

	do_read();
}

void ssl_http_session::on_shutdown(beast::error_code ec)
{
	if(ec)
		return fail(ec, "shutdown");

	// At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------

// Launch the detector
void
detect_session::run()
{
	// We need to be executing within a strand to perform async operations
	// on the I/O objects in this session. Although not strictly necessary
	// for single-threaded contexts, this example code is written to be
	// thread-safe by default.
	net::dispatch(
		stream_.get_executor(),
		beast::bind_front_handler(
			&detect_session::on_run,
			this->shared_from_this()));
}

void
detect_session::on_run()
{
	// Set the timeout.
	stream_.expires_after(std::chrono::seconds(30));

	beast::async_detect_ssl(
		stream_,
		buffer_,
		beast::bind_front_handler(
			&detect_session::on_detect,
			this->shared_from_this()));
}

void
detect_session::on_detect(beast::error_code ec, bool result)
{
	if(ec)
		return fail(ec, "detect");

	if(result)
	{
		// Launch SSL session
		std::make_shared<ssl_http_session>(
			std::move(stream_),
			ctx_,
			std::move(buffer_),
			doc_root_,
			mtm_)->run();
		return;
	}

	// Launch plain session
	std::make_shared<plain_http_session>(
		std::move(stream_),
		std::move(buffer_),
		doc_root_,
		mtm_)->run();
}

// Accepts incoming connections and launches the sessions
listener::listener(
	net::io_context& ioc,
	ssl::context& ctx,
	tcp::endpoint endpoint,
	std::shared_ptr<std::string const> const& doc_root,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
	: ioc_(ioc)
	, ctx_(ctx)
	, acceptor_(net::make_strand(ioc))
	, doc_root_(doc_root)
	, mtm_(mtm)
{
	beast::error_code ec;

	// Open the acceptor
	acceptor_.open(endpoint.protocol(), ec);
	if(ec)
	{
		fail(ec, "open");
		return;
	}

	// Allow address reuse
	acceptor_.set_option(net::socket_base::reuse_address(true), ec);
	if(ec)
	{
		fail(ec, "set_option");
		return;
	}

	// Bind to the server address
	acceptor_.bind(endpoint, ec);
	if(ec)
	{
		fail(ec, "bind");
		return;
	}

	// Start listening for connections
	acceptor_.listen(
		net::socket_base::max_listen_connections, ec);
	if(ec)
	{
		fail(ec, "listen");
		return;
	}
}

// Start accepting incoming connections
void
listener::run()
{
	do_accept();
}

void
listener::do_accept()
{
	// The new connection gets its own strand
	acceptor_.async_accept(
		net::make_strand(ioc_),
		beast::bind_front_handler(
			&listener::on_accept,
			shared_from_this()));
}

void
listener::on_accept(beast::error_code ec, tcp::socket socket)
{
	if(ec)
	{
		fail(ec, "accept");
	}
	else
	{
		// Create the detector http_session and run it
		std::make_shared<detect_session>(
			std::move(socket),
			ctx_,
			doc_root_,
			mtm_)->run();
	}

	// Accept another connection
	do_accept();
}

} // namespace session
