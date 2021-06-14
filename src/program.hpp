#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "fuse.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

class program final {
public:
	program(fuse_args *args);
	program(program const &) = delete;
	~program();

public:
	program &operator=(program const &) = delete;

public:
	void init_forked();
	void init();
	void run();

private:
	void on_signal(boost::system::error_code const &ec, int sig);

private:
	boost::asio::io_context io;
	fuse fs;
	boost::asio::signal_set sig;
	bool initialized;
};

#endif // PROGRAM_HPP
