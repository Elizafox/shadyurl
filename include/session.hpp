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
// Example: HTTP SSL server
//
//------------------------------------------------------------------------------

#ifndef SESSION_H
#define SESSION_H

#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#	define BOOST_BEAST_USE_STD_STRING_VIEW
#endif

#include <boost/beast/ssl.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <tuple>
#include <regex>
#include <utility>

#include "log.hpp"
#include "request.hpp"
#include "server_state.hpp"

namespace session
{

namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;		// from <boost/beast/http.hpp>
namespace net = boost::asio;		// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;	// from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;	// from <boost/asio/ip/tcp.hpp>

// Handles an HTTP server connection.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template<class Derived>
class http_session
{
	// Access the derived class, this is part of
	// the Curiously Recurring Template Pattern idiom.
	Derived&
	derived()
	{
		return static_cast<Derived&>(*this);
	}

	// This queue is used for HTTP pipelining.
	class queue
	{
		enum
		{
			// Maximum number of responses we will queue
			limit = 8
		};

		// The type-erased, saved work item
		struct work
		{
			virtual ~work() = default;
			virtual void operator()() = 0;
		};

		http_session& self_;
		std::vector<std::unique_ptr<work>> items_;

	public:
		explicit
		queue(http_session& self)
			: self_(self)
		{
			static_assert(limit > 0, "queue limit must be positive");
			items_.reserve(limit);
		}

		// Returns `true` if we have reached the queue limit
		bool
		is_full() const
		{
			return items_.size() >= limit;
		}

		// Called when a message finishes sending
		// Returns `true` if the caller should initiate a read
		bool
		on_write()
		{
			BOOST_ASSERT(!items_.empty());
			auto const was_full = is_full();
			items_.erase(items_.begin());
			if(!items_.empty())
				(*items_.front())();
			return was_full;
		}

		// Called by the HTTP handler to send a response.
		template<bool isRequest, class Body, class Fields>
		void
		operator()(http::message<isRequest, Body, Fields>&& msg)
		{
			// This holds a work item
			struct work_impl : work
			{
				http_session& self_;
				http::message<isRequest, Body, Fields> msg_;

				work_impl(
					http_session& self,
					http::message<isRequest, Body, Fields>&& msg)
					: self_(self)
					, msg_(std::move(msg))
				{
				}

				void
				operator()()
				{
					http::async_write(
						self_.derived().stream(),
						msg_,
						beast::bind_front_handler(
							&http_session::on_write,
							self_.derived().shared_from_this(),
							msg_.need_eof()));
				}
			};

			// Allocate and store the work
			items_.push_back(
				std::make_unique<work_impl>(self_, std::move(msg)));

			// If there was no previous work, start this one
			if(items_.size() == 1)
				(*items_.front())();
		}
	};

	const server_state::ServerState& state_;
	queue queue_;

	// The parser is stored in an optional container so we can
	// construct it from scratch it at the beginning of each new message.
	std::optional<http::request_parser<http::string_body>> parser_;

protected:
	beast::flat_buffer buffer_;

public:
	// Construct the session
	http_session(
		beast::flat_buffer buffer,
		const server_state::ServerState& state)
		: state_(state)
		, queue_(*this)
		, buffer_(std::move(buffer))
	{
	}

	void
	do_read()
	{
		// Construct a new parser for each message
		parser_.emplace();

		// Apply a reasonable limit to the allowed size
		// of the body in bytes to prevent abuse.
		parser_->body_limit(10000);

		// Set the timeout.
		beast::get_lowest_layer(
			derived().stream()).expires_after(std::chrono::seconds(30));

		// Read a request using the parser-oriented interface
		http::async_read(
			derived().stream(),
			buffer_,
			*parser_,
			beast::bind_front_handler(
				&http_session::on_read,
				derived().shared_from_this()));
	}

	void
	on_read(beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// This means they closed the connection
		if(ec == http::error::end_of_stream)
			return derived().do_eof();

		if(ec)
			return logging::fail(ec, "read");

		// Send the response
		request::handle_request(state_, parser_->release(), queue_);

		// If we aren't at the queue limit, try to pipeline another request
		if(!queue_.is_full())
			do_read();
	}

	void
	on_write(bool close, beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if(ec)
			return logging::fail(ec, "write");

		if(close)
		{
			// This means we should close the connection, usually because
			// the response indicated the "Connection: close" semantic.
			return derived().do_eof();
		}

		// Inform the queue that a write completed
		if(queue_.on_write())
		{
			// Read another request
			do_read();
		}
	}
};

//------------------------------------------------------------------------------

// Handles a plain HTTP connection
class plain_http_session
	: public http_session<plain_http_session>
	, public std::enable_shared_from_this<plain_http_session>
{
	beast::tcp_stream stream_;
public:
	// Create the session
	plain_http_session(
		beast::tcp_stream&& stream,
		beast::flat_buffer&& buffer,
		const server_state::ServerState& state)
		: http_session<plain_http_session>(
			std::move(buffer),
			state)
		, stream_(std::move(stream))
	{
	}
	void run();
	beast::tcp_stream& stream();
	beast::tcp_stream release_stream();
	void do_eof();
};

//------------------------------------------------------------------------------

// Handles an SSL HTTP connection
class ssl_http_session
	: public http_session<ssl_http_session>
	, public std::enable_shared_from_this<ssl_http_session>
{
	beast::ssl_stream<beast::tcp_stream> stream_;

public:
	// Create the http_session
	ssl_http_session(
		beast::tcp_stream&& stream,
		ssl::context& ctx,
		beast::flat_buffer&& buffer,
		const server_state::ServerState& state)
		: http_session<ssl_http_session>(
			std::move(buffer),
			state)
		, stream_(std::move(stream), ctx)
	{
	}

	void run();
	beast::ssl_stream<beast::tcp_stream>& stream();
	beast::ssl_stream<beast::tcp_stream> release_stream();
	void do_eof();
private:
	void on_handshake(beast::error_code, std::size_t);
	void on_shutdown(beast::error_code);
};

//------------------------------------------------------------------------------

// Detects SSL handshakes
class detect_session : public std::enable_shared_from_this<detect_session>
{
	beast::tcp_stream stream_;
	ssl::context& ctx_;
	const server_state::ServerState& state_;
	beast::flat_buffer buffer_;
public:
	explicit
	detect_session(
		tcp::socket&& socket,
		ssl::context& ctx,
		const server_state::ServerState& state)
		: stream_(std::move(socket))
		, ctx_(ctx)
		, state_(state)
	{
	}

	void run();
	void on_run();
	void on_detect(beast::error_code, bool);

};

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
	net::io_context& ioc_;
	ssl::context& ctx_;
	tcp::acceptor acceptor_;
	const server_state::ServerState& state_;

public:
	listener(
		net::io_context&,
		ssl::context&,
		tcp::endpoint,
		const server_state::ServerState&);
	void run();
private:
	void do_accept();
	void on_accept(beast::error_code, tcp::socket);
};

} // namespace session

#endif // SESSION_H
