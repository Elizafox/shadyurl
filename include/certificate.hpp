#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <boost/asio/ssl/context.hpp>

#include "server_state.hpp"

namespace certificate
{

bool load_server_certificate(const server_state::ServerState&, boost::asio::ssl::context&);

} // namespace certificate

#endif
