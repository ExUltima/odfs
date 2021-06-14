#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

class client final {
public:
	client(boost::asio::io_context &ctx);
	client(client const &) = delete;
	~client();

public:
	client &operator=(client const &) = delete;

private:
	boost::asio::ssl::context ssl;
	boost::beast::ssl_stream<boost::beast::tcp_stream> con;
};

#endif // CLIENT_HPP
