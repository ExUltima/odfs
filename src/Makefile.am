bin_PROGRAMS = mount.onedrive
mount_onedrive_LDFLAGS = $(BOOST_LDFLAGS)
mount_onedrive_LDADD = \
	$(BOOST_ASIO_LIB) \
	$(CURL_LIBS) \
	$(FUSE_LIBS) \
	$(MICROHTTPD_LIBS)
mount_onedrive_SOURCES = \
	auth.cpp \
	auth.hpp \
	client.cpp \
	client.hpp \
	fs.cpp \
	fs.hpp \
	io.cpp \
	io.hpp \
	main.cpp \
	option.cpp \
	option.hpp
mount_onedrive_CPPFLAGS = \
	-DFUSE_USE_VERSION=31 \
	-D_FILE_OFFSET_BITS=64 \
	$(BOOST_CPPFLAGS)
mount_onedrive_CXXFLAGS = \
	$(CURL_CFLAGS) \
	$(FUSE_CFLAGS) \
	$(MICROHTTPD_CFLAGS)
