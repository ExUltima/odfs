#include "program.hpp"
#include "auth.hpp"

#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>
#include <boost/system/system_error.hpp>

#include <cerrno>
#include <csignal>
#include <ostream>
#include <stdexcept>
#include <system_error>

#include <unistd.h>

program::program(fuse_args *args) :
	io(1),
	fs(io, args),
	sig(io, SIGINT, SIGTERM),
	initialized(false)
{
}

program::~program()
{
	if (initialized) {
		auth_term();
	}
}

void program::init_forked()
{
	if (setsid() < 0) {
		throw std::system_error(errno, std::system_category(), "failed to start a new session");
	}

	if (chdir("/") < 0) {
		throw std::system_error(errno, std::system_category(), "failed to change directory to /");
	}

	init();
}

void program::init()
{
	sig.async_wait(
		boost::bind(
			&program::on_signal,
			this,
			boost::asio::placeholders::error(),
			boost::asio::placeholders::signal_number()));

	fs.mount();

	if (!auth_init()) {
		throw std::runtime_error("failed to initialize authentication system");
	}

	initialized = true;
}

void program::run()
{
	while (!fs.is_exit()) {
		io.run_one();
	}
}

void program::on_signal(boost::system::error_code const &ec, int sig)
{
	if (ec) {
		// no need to check boost::asio::error::operation_aborted since we own the message loop
		throw boost::system::system_error(ec, "error while waiting for signals");
	} else {
		fs.exit();
	}
}
