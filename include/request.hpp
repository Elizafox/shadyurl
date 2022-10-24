#ifndef REQUEST_H
#define REQUEST_H

#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#	define BOOST_BEAST_USE_STD_STRING_VIEW
#endif

#include <syslog.h>
#include <stdarg.h>
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

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <inja/inja.hpp>

#include "generate.hpp"
#include "mime.hpp"
#include "parseqs.hpp"
#include "path.hpp"
#include "sqlite_helper.hpp"
#include "multipart_wrapper.hpp"
#include "server_state.hpp"


namespace request
{

namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;		// from <boost/beast/http.hpp>

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
	const server_state::ServerState& state)
{
	http::response<http::empty_body> res{http::status::ok, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, get_mime_type(path, state.get_mime_type_map()));
	res.content_length(body.size());
	res.keep_alive(req.keep_alive());
	return res;
}

auto ok_get_file(
	const auto& req,
	std::string_view path,
	http::file_body::value_type&& body,
	const server_state::ServerState& state)
{
	// Cache the size since we need it after the move
	auto const size = body.size();

	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(body)),
		std::make_tuple(http::status::ok, req.version())};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, get_mime_type(path, state.get_mime_type_map()));
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
	const server_state::ServerState& state,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	// Build the path to the requested file
	std::string path = path_cat(state.get_config_doc_root(), req.target());

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
	{
		syslog(LOG_ERR, "Unknown error fetching file %s: %s", req.target().data(), ec.message().c_str());
		return send(server_error(req, ec.message()));
	}

	// Respond to HEAD request
	if(req.method() == http::verb::head)
	{
		return send(ok_head_file(req, path, std::move(body), state));
	}
	else if(req.method() == http::verb::get)
	{
		return send(ok_get_file(req, path, std::move(body), state));
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
	const server_state::ServerState& state,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	if(req.method() != http::verb::post)
	{
		// POST requests only please!
		return send(bad_request(req, "Unknown HTTP-method"));
	}

	std::string path = path_cat(state.get_config_doc_root(), req.target());

	if(req.target().back() == '/')
		path.append("index.html");

	// These are templated pages
	// Off to the templating engine
	inja::Environment env;
	inja::json data;

	data["hostname"] = state.get_config_hostname();

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

	auto db = sqlite_helper::make_sqlite3_handle(state.get_config_db_path().data());
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(
		db.get(),
		"INSERT INTO urls (token, url) VALUES (?, ?);",
		-1,
		&res,
		nullptr);
	if(rc != SQLITE_OK)
	{
		std::string error{sqlite3_errmsg(db.get())};
		syslog(LOG_ERR, "Error with sqlite3: %s", error.c_str());
		return send(server_error(req, "SQL error: " + error));
	}

	sqlite_helper::scope_exit cleanup{[&] { sqlite3_finalize(res); }};

	sqlite3_bind_text(res, 1, token.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(res, 2, url.c_str(), -1, SQLITE_STATIC);

	int step = sqlite3_step(res);
	if(step != SQLITE_DONE)
	{
		std::string error{sqlite3_errmsg(db.get())};
		syslog(LOG_ERR, "Error with sqlite3: %s", error.c_str());
		return send(server_error(req, "SQL error: " + error));
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
	const server_state::ServerState& state,
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
	std::string path = path_cat(state.get_config_doc_root(), req.target());

	if(req.target().back() == '/')
		path.append("index.html");

	inja::Environment env;
	inja::json data;

	data["hostname"] = state.get_config_hostname();

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
	const server_state::ServerState& state,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	if(req.method() != http::verb::get)
	{
		// GET requests only please!
		return send(bad_request(req, "Unknown HTTP-method"));
	}

	// We assume this is a shortened URL otherwise.
	auto db = sqlite_helper::make_sqlite3_handle(state.get_config_db_path().data());
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(
		db.get(),
		"SELECT url FROM urls WHERE token = (?);",
		-1,
		&res,
		nullptr);
	if(rc != SQLITE_OK)
	{
		std::string error{sqlite3_errmsg(db.get())};
		syslog(LOG_ERR, "Error with sqlite3: %s", error.c_str());
		return send(server_error(req, "SQL error: " + error));
	}

	sqlite_helper::scope_exit cleanup{[&] { sqlite3_finalize(res); }};

	std::string token{req.target().substr(1)};
	rc = sqlite3_bind_text(res, 1, token.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK)
	{
		std::string error{sqlite3_errmsg(db.get())};
		syslog(LOG_ERR, "Error with sqlite3: %s", error.c_str());
		return send(server_error(req, "SQL error:" + error));
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
	const server_state::ServerState& state,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send)
{
	using fn_ptr_type =
		void(*)(
			const server_state::ServerState& state,
			http::request<Body, http::basic_fields<Allocator>>&&,
			Send&&);
	const std::vector<std::tuple<std::regex, fn_ptr_type>> routes{
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
				state,
				std::forward<decltype(req)>(req),
				std::forward<decltype(send)>(send));
		}
	}

	return send(not_found(req, target));
}

} // namespace request

#endif // REQUEST_H
