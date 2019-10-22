bin_PROGRAMS = mount.onedrive
mount_onedrive_LDADD = $(FUSE_LIBS) $(MICROHTTPD_LIBS)
mount_onedrive_SOURCES = \
	auth.c \
	auth.h \
	dispatcher.c \
	dispatcher.h \
	fs.c \
	fs.h \
	main.c \
	option.c \
	option.h
mount_onedrive_CFLAGS = \
	-DFUSE_USE_VERSION=31 \
	-D_FILE_OFFSET_BITS=64 \
	$(FUSE_CFLAGS) \
	$(MICROHTTPD_CFLAGS)
