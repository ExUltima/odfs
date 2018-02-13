bin_PROGRAMS = mount.onedrive
mount_onedrive_LDADD = $(FUSE_LIBS)
mount_onedrive_SOURCES = \
	main.c \
	option.c
mount_onedrive_CFLAGS = \
	-DFUSE_USE_VERSION=31 \
	-D_FILE_OFFSET_BITS=64 $(FUSE_CFLAGS)
