#include "fuse.hpp"
#include "fs.hpp"
#include "option.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>
#include <boost/system/system_category.hpp>
#include <boost/system/system_error.hpp>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <fuse_common.h>
#include <fuse_lowlevel.h>

static const struct fuse_lowlevel_ops ops = {
	.init = fs_init,
	.destroy = fs_destroy
};

fuse::fuse(boost::asio::io_context &ctx, fuse_args *args) :
	fd(ctx)
{
	ses = fuse_session_new(args, &ops, sizeof(ops), nullptr);
	if (!ses) {
		throw std::runtime_error("failed to initialize FUSE");
	}
}

fuse::~fuse()
{
	if (fd.is_open()) {
		fd.cancel();
		fd.release();

		fuse_session_unmount(ses);
	}

	fuse_session_destroy(ses);
}

bool fuse::is_exit()
{
	return fuse_session_exited(ses) != 0;
}

void fuse::mount()
{
	if (fuse_session_mount(ses, option_get()->fuse.mountpoint) < 0) {
		throw std::runtime_error("mount failed");
	}

	try {
		fd.assign(fuse_session_fd(ses));
	} catch (...) {
		fuse_session_unmount(ses);
		throw;
	}

	begin_read();
}

void fuse::exit()
{
	fuse_session_exit(ses);
}

void fuse::begin_read()
{
	fd.async_wait(
		boost::asio::posix::descriptor_base::wait_read,
		boost::bind(&fuse::on_read, this, boost::asio::placeholders::error()));
}

void fuse::on_read(boost::system::error_code const &ec)
{
	if (ec) {
		if (ec == boost::asio::error::operation_aborted) {
			return;
		}
		throw boost::system::system_error(ec, "error while waiting for FUSE message");
	}

	// read message
	fuse_buf buf;
	int r;

	std::memset(&buf, 0, sizeof(buf));

	r = fuse_session_receive_buf(ses, &buf);

	if (r <= 0) {
		if (r < 0) {
			auto msg = "failed to read FUSE message";
			throw boost::system::system_error(std::abs(r), boost::system::system_category(), msg);
		} else {
			exit();
		}
		return;
	}

	// process message
	fuse_session_process_buf(ses, &buf);
	std::free(buf.mem);

	begin_read();
}
