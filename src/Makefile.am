bin_PROGRAMS = mount.odfs
mount_odfs_SOURCES = main.c option.c
mount_odfs_CFLAGS = -DFUSE_USE_VERSION=31 -D_FILE_OFFSET_BITS=64 $(FUSE_CFLAGS)
mount_odfs_LDADD = $(FUSE_LIBS)
