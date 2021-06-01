bin_PROGRAMS = mount.onedrive
mount_onedrive_LDADD = $(CURL_LIBS) $(FUSE_LIBS) $(MICROHTTPD_LIBS)
mount_onedrive_SOURCES = \
	auth.cpp \
	auth.hpp \
	client.cpp \
	client.hpp \
	dispatcher.cpp \
	dispatcher.hpp \
	fs.cpp \
	fs.hpp \
	main.cpp \
	option.cpp \
	option.hpp
mount_onedrive_CXXFLAGS = \
	-DFUSE_USE_VERSION=31 \
	-D_FILE_OFFSET_BITS=64 \
	$(CURL_CFLAGS) \
	$(FUSE_CFLAGS) \
	$(MICROHTTPD_CFLAGS)
