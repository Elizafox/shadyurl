#ifndef SERVER_CERTIFICATE_H
#define SERVER_CERTIFICATE_H

#include <boost/asio/ssl/context.hpp>

void load_server_certificate(boost::asio::ssl::context&);

#endif
