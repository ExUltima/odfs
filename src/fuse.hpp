#ifndef FUSE_HPP
#define FUSE_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/system/error_code.hpp>

#include <fuse_opt.h>

class fuse final {
public:
	fuse(boost::asio::io_context &ctx, fuse_args *args);
	fuse(fuse const &) = delete;
	~fuse();

public:
	fuse &operator=(fuse const &) = delete;

public:
	bool is_exit();

public:
	void mount();
	void exit();

private:
	void begin_read();
	void on_read(boost::system::error_code const &ec);

private:
	boost::asio::posix::stream_descriptor fd;
	struct fuse_session *ses;
};

#endif // FUSE_HPP
