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

#include "request.hpp"

namespace session
{

namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;		// from <boost/beast/http.hpp>
namespace net = boost::asio;		// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;	// from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;	// from <boost/asio/ip/tcp.hpp>

void fail(beast::error_code, char const*);

// Various response types

// Returns a bad request response
auto bad_request(const auto& req, std::string_view why)
{
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = std::string(why);
	res.prepare_payload();
	return res;
}

// Returns a not found response
auto not_found(const auto& req, std::string_view target)
{
	http::response<http::string_body> res{http::status::not_found, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = "The resource '" + std::string(target) + "' was not found.";
	res.prepare_payload();
	return res;
}

// Returns a server error response
auto server_error(const auto& req, std::string_view what)
{
	http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = "An error occurred: '" + std::string(what) + "'";
	res.prepare_payload();
	return res;
}

// Returns a permanent redirect
auto redirect_permanent(const auto& req, std::string_view url)
{
	http::response<http::empty_body> res{http::status::moved_permanently, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::location, url);
	res.prepare_payload();
	return res;
}

auto ok_head_file(
	const auto& req,
	std::string_view path,
	http::file_body::value_type&& body,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
{
	http::response<http::empty_body> res{http::status::ok, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, get_mime_type(path, mtm));
	res.content_length(body.size());
	res.keep_alive(req.keep_alive());
	return res;
}

auto ok_get_file(
	const auto& req,
	std::string_view path,
	http::file_body::value_type&& body,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
{
	// Cache the size since we need it after the move
	auto const size = body.size();

	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(body)),
		std::make_tuple(http::status::ok, req.version())};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, get_mime_type(path, mtm));
	res.content_length(size);
	res.keep_alive(req.keep_alive());
	return res;
}

auto ok_string(
	const auto& req,
	std::string_view data)
{
	http::response<http::string_body> res{http::status::ok, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = data;
	res.prepare_payload();
	return res;
}

auto bad_request_string(
	const auto& req,
	std::string_view data)
{
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = data;
	res.prepare_payload();
	return res;
}

// Produce an HTTP response for a file request
template<class Body, class Allocator, class Send>
void
handle_file(
	std::string_view doc_root,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	// Build the path to the requested file
	std::string path = path_cat(doc_root, req.target());

	// Make sure we can handle the method
	if(req.method() != http::verb::get &&
		req.method() != http::verb::head)
	{
		return send(bad_request(req, "Unknown HTTP-method"));
	}

	// Attempt to open the file
	beast::error_code ec;
	http::file_body::value_type body;
	body.open(path.c_str(), beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if(ec == beast::errc::no_such_file_or_directory)
		return send(not_found(req, req.target()));

	// Handle an unknown error
	if(ec)
		return send(server_error(req, ec.message()));

	// Respond to HEAD request
	if(req.method() == http::verb::head)
	{
		return send(ok_head_file(req, path, std::move(body), mtm));
	}
	else if(req.method() == http::verb::get)
	{
		return send(ok_get_file(req, path, std::move(body), mtm));
	}
	else
	{
		return send(bad_request(req, "Unknown HTTP method"));
	}
}

// Produce an HTTP response for a post request
template<class Body, class Allocator, class Send>
void
handle_post(
	std::string_view doc_root,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	if(req.method() != http::verb::post)
	{
		// POST requests only please!
		return send(bad_request(req, "Unknown HTTP-method"));
	}

	std::string path = path_cat(doc_root, req.target());

	if(req.target().back() == '/')
		path.append("index.html");

	// These are templated pages
	// Off to the templating engine
	inja::Environment env;
	inja::json data;
	std::string url;

	std::string_view content_type = req["Content-Type"];

	if(content_type == "application/x-www-form-urlencoded")
	{
		std::map qsm = parseqs::parse_qsl(req.body());
		auto it = qsm.find("url");
		if(it == qsm.end())
		{
			return send(bad_request(req, "No URL parameter passed"));
		}

		url = it->second;
	}
	else if(content_type.starts_with("multipart/form-data"))
	{
		using MultiPartData = multipart_wrapper::MultiPartData;
		using MultiPartSection = MultiPartData::MultiPartSection;
		auto hv = MultiPartSection::parse_header_value(content_type);

		if(std::holds_alternative<std::string>(hv))
		{
			return send(bad_request(req, "Bad request"));
		}

		auto hvmap = std::get<MultiPartSection::header_params_type>(hv);
		std::string boundary{hvmap["boundary"]};
		if(boundary.empty())
		{
			return send(bad_request(req, "Bad request"));
		}

		MultiPartData mpd{boundary};
		std::string body{req.body()};
		boost::trim_right(body);
		mpd.ingest(body);
		auto form_data = mpd.get_data();
		for(auto& elem : form_data)
		{
			auto h = elem.get_headers();
			auto cd = h["Content-Disposition"];
			if(std::holds_alternative<std::string>(cd))
			{
				continue;
			}

			auto cdmap = std::get<MultiPartSection::header_params_type>(cd);
			if(cdmap["name"] == "url")
			{
				url = elem.get_data();
				break;
			}
		}

		if(url == "")
		{
			return send(bad_request(req, "No URL specified"));
		}
	}
	else
	{
		return send(bad_request(req, "Bad content type " + std::string(content_type)));
	}

	if(!(url.starts_with("http://") || url.starts_with("https://")))
	{
		return send(bad_request(req, "Invalid URL"));
	}

	std::string token = generate::generate_random_filename();
	data["url"] = url;
	data["token"] = token;

	auto db = sqlite_helper::make_sqlite3_handle("urls.db");
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(
		db.get(),
		"INSERT INTO urls (token, url) VALUES (?, ?);",
		-1,
		&res,
		nullptr);
	if(rc != SQLITE_OK)
	{
		return send(server_error(req, "SQL error:" + std::string(sqlite3_errmsg(db.get()))));
	}

	sqlite_helper::scope_exit cleanup{[&] { sqlite3_finalize(res); }};

	sqlite3_bind_text(res, 1, token.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(res, 2, url.c_str(), -1, SQLITE_STATIC);

	int step = sqlite3_step(res);
	if(step != SQLITE_DONE)
	{
		return send(server_error(req, "SQL error:" + std::string(sqlite3_errmsg(db.get()))));
	}

	inja::Template temp;
	std::string result;

	try
	{
		temp = env.parse_template(path);
		result = env.render(temp, data);
	}
	catch(std::exception& e)
	{
		return send(bad_request(req, std::string("Could not serve page: ") + e.what()));
	}

	return send(ok_string(req, result));
}

// Handle serving a template
template<class Body, class Allocator, class Send>
void
handle_get_template(
	std::string_view doc_root,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	if(req.method() != http::verb::get)
	{
		// GET requests only please!
		return send(bad_request(req, "Unknown HTTP-method"));
	}

	// These are templated pages
	// Off to the templating engine
	std::string path = path_cat(doc_root, req.target());

	if(req.target().back() == '/')
		path.append("index.html");

	inja::Environment env;
	inja::json data;

	inja::Template temp;
	std::string result;
	try
	{
		temp = env.parse_template(path);
		result = env.render(temp, data);
	}
	catch(std::exception& e)
	{
		return send(bad_request(req, std::string("Could not serve page: ") + e.what()));
	}

	return send(ok_string(req, result));
}

// Handle getting a shortened/shady URL
template<class Body, class Allocator, class Send>
void
handle_get_url(
	std::string_view doc_root,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	if(req.method() != http::verb::get)
	{
		// GET requests only please!
		return send(bad_request(req, "Unknown HTTP-method"));
	}

	// We assume this is a shortened URL otherwise.
	auto db = sqlite_helper::make_sqlite3_handle("urls.db");
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(
		db.get(),
		"SELECT url FROM urls WHERE token = (?);",
		-1,
		&res,
		nullptr);
	if(rc != SQLITE_OK)
	{
		return send(server_error(req, "SQL error:" + std::string(sqlite3_errmsg(db.get()))));
	}

	sqlite_helper::scope_exit cleanup{[&] { sqlite3_finalize(res); }};

	std::string token{req.target().substr(1)};
	rc = sqlite3_bind_text(res, 1, token.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK)
	{
		return send(server_error(req, "SQL error:" + std::string(sqlite3_errmsg(db.get()))));
	}

	std::string url;
	int step = sqlite3_step(res);
	if(step == SQLITE_ROW)
	{
		url = reinterpret_cast<const char*>(sqlite3_column_text(res, 0));
	}
	else
	{
		// ;)
		url = "https://www.youtube.com/watch?v=dQw4w9WgXcQ?autoplay=1";
	}
	return send(redirect_permanent(req, url));
}

#define REQ_ROUTE_DEF(re, fn) {std::regex{re}, static_cast<fn_ptr_type>(&fn)}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Body, class Allocator, class Send>
void
handle_request(
	std::string_view doc_root,
	std::shared_ptr<mime_type::MimeTypeMap const> const& mtm,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	using fn_ptr_type =
		void(*)(
			std::string_view,
			std::shared_ptr<mime_type::MimeTypeMap const> const&,
			http::request<Body, http::basic_fields<Allocator>>&&,
			Send&&);
	static const std::vector<std::tuple<std::regex, fn_ptr_type>> routes{
		REQ_ROUTE_DEF(R"RE(^/(assets/.*|favicon\.ico|robots\.txt)$)RE", handle_file),
		REQ_ROUTE_DEF(R"RE(^/post\.html$)RE", handle_post),
		REQ_ROUTE_DEF(R"RE(^/$)RE", handle_get_template),
		REQ_ROUTE_DEF(R"RE(^/(.*\.html)?$)RE", handle_get_template),
		REQ_ROUTE_DEF(R"RE(^/[^/]+$)RE", handle_get_url),
	};

	// Request path must be absolute and not contain "..".
	if(req.target().empty() ||
		req.target()[0] != '/' ||
		req.target().find("..") != std::string_view::npos)
	{
		return send(bad_request(req, "Illegal request-target"));
	}

	// std::regex_search only takes strings
	std::string target{req.target()};
	for(auto& [re, fn] : routes)
	{
		if(std::regex_search(target, re))
		{
			return fn(
				doc_root,
				mtm,
				std::forward<decltype(req)>(req),
				std::forward<decltype(send)>(send));
		}
	}

	return send(not_found(req, target));
}

//------------------------------------------------------------------------------

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

	std::shared_ptr<std::string const> doc_root_;
	std::shared_ptr<mime_type::MimeTypeMap const> mtm_;
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
		std::shared_ptr<std::string const> const& doc_root,
		std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
		: doc_root_(doc_root)
		, mtm_(mtm)
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
			return fail(ec, "read");

		// Send the response
		handle_request(*doc_root_, mtm_, parser_->release(), queue_);

		// If we aren't at the queue limit, try to pipeline another request
		if(!queue_.is_full())
			do_read();
	}

	void
	on_write(bool close, beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if(ec)
			return fail(ec, "write");

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
		std::shared_ptr<std::string const> const& doc_root,
		std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
		: http_session<plain_http_session>(
			std::move(buffer),
			doc_root,
			mtm)
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
		std::shared_ptr<std::string const> const& doc_root,
		std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
		: http_session<ssl_http_session>(
			std::move(buffer),
			doc_root,
			mtm)
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
	std::shared_ptr<std::string const> doc_root_;
	std::shared_ptr<mime_type::MimeTypeMap const> mtm_;
	beast::flat_buffer buffer_;
public:
	explicit
	detect_session(
		tcp::socket&& socket,
		ssl::context& ctx,
		std::shared_ptr<std::string const> const& doc_root,
		std::shared_ptr<mime_type::MimeTypeMap const> const& mtm)
		: stream_(std::move(socket))
		, ctx_(ctx)
		, doc_root_(doc_root)
		, mtm_(mtm)
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
	std::shared_ptr<std::string const> doc_root_;
	std::shared_ptr<mime_type::MimeTypeMap const> mtm_;

public:
	listener(
		net::io_context&,
		ssl::context&,
		tcp::endpoint,
		std::shared_ptr<std::string const> const&,
		std::shared_ptr<mime_type::MimeTypeMap const> const&);
	void run();
private:
	void do_accept();
	void on_accept(beast::error_code, tcp::socket);

};

} // namespace session

#endif // SESSION_H
