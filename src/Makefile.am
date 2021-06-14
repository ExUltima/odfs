bin_PROGRAMS = mount.onedrive
mount_onedrive_LDFLAGS = $(BOOST_LDFLAGS)
mount_onedrive_LDADD = \
	$(BOOST_ASIO_LIB) \
	$(FUSE_LIBS) \
	$(MICROHTTPD_LIBS) \
	$(OPENSSL_LIBS)
mount_onedrive_SOURCES = \
	auth.cpp \
	auth.hpp \
	client.cpp \
	client.hpp \
	fs.cpp \
	fs.hpp \
	fuse.cpp \
	fuse.hpp \
	main.cpp \
	option.cpp \
	option.hpp \
	program.cpp \
	program.hpp
mount_onedrive_CPPFLAGS = \
	-DFUSE_USE_VERSION=31 \
	-D_FILE_OFFSET_BITS=64 \
	$(BOOST_CPPFLAGS)
mount_onedrive_CXXFLAGS = \
	$(FUSE_CFLAGS) \
	$(MICROHTTPD_CFLAGS) \
	$(OPENSSL_CFLAGS)
