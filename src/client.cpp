#include "client.hpp"

client::client(boost::asio::io_context &ctx) :
	ssl(boost::asio::ssl::context::tlsv13_client),
	con(ctx, ssl)
{
	ssl.set_verify_mode(boost::asio::ssl::context::verify_peer);
}

client::~client()
{
}
