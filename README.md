# OneDrive Filesystem Driver for Linux

This is a filesystem driver for Microsoft OneDrive for Linux implementing with
FUSE.

## Build

```sh
./autogen.sh && ./configure && make
```

## Install

```sh
make install
```

## Running

Create an empty directory for mounting then run the following command:

```sh
mount.onedrive MOUNTPATH
```

Open a web browser to http://localhost:2300 and sign in with OneDrive account
you want to mount.
