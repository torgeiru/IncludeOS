#pragma once
#ifndef VIRTIO_FILESYSTEM_DEFS_HPP
#define VIRTIO_FILESYSTEM_DEFS_HPP

#include <fuse/fuse.hpp>

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION_MIN 36

/* INIT request and response */
typedef struct __attribute__((packed)) virtio_fs_init_req {
  fuse_in_header in_header;
  fuse_init_in init_in;

  virtio_fs_init_req(uint32_t majo, uint32_t mino, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_init_in), FUSE_INIT, uniqu, nodei),
    init_in(majo, mino) {}
} virtio_fs_init_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_init_out init_out; // out.len - sizeof(fuse_out_header)
} virtio_fs_init_res;

/* LOOKUP request and response */
typedef struct __attribute__((packed)) virtio_fs_lookup_req {
  fuse_in_header in_header;

  virtio_fs_lookup_req(uint32_t plen, uint64_t uniqu, uint64_t nodei) 
  : in_header(plen, FUSE_LOOKUP, uniqu, nodei) {}
} virtio_fs_lookup_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_entry_param entry_param;
} virtio_fs_lookup_res;

/* OPEN request and response */
typedef struct __attribute__((packed)) virtio_fs_open_req {
  fuse_in_header in_header;
  fuse_open_in open_in;

  virtio_fs_open_req(uint32_t flag, uint32_t open_flag, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_open_in), FUSE_OPEN, uniqu, nodei), 
    open_in(flag, open_flag) {}
} virtio_fs_open_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_open_out open_out;
} virtio_fs_open_res;

/* CREAT request and response */
typedef struct __attribute__((packed)) virtio_fs_creat_req {
  fuse_in_header in_header;
  fuse_creat_in create_in;

  virtio_fs_creat_req(uint32_t pathname_len,uint32_t flag, uint32_t mod, uint64_t uniqu, uint64_t nodei) 
  : in_header(sizeof(fuse_creat_in) + pathname_len + 1, FUSE_CREATE, uniqu, nodei), create_in(flag, mod) {}
} virtio_fs_creat_req;

typedef struct __attribute__((packed)) virtio_fs_creat_res {
  fuse_out_header out_header;
  fuse_entry_param entry_param;
  fuse_open_out open_out;
} virtio_fs_creat_res;

/* READ request and response */
typedef struct __attribute__((packed)) virtio_fs_read_req {
  fuse_in_header in_header;
  fuse_read_in read_in;

  virtio_fs_read_req(uint64_t f, uint64_t offse, uint32_t siz, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_read_in), FUSE_READ, uniqu, nodei),
    read_in(f, offse, siz, 0, 0) {} 
} virtio_fs_read_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
} virtio_fs_read_res;

/* WRITE request and response */
typedef struct __attribute__((packed)) virtio_fs_write_req {
  fuse_in_header in_header;
  fuse_write_in write_in;

  virtio_fs_write_req(uint64_t f, uint64_t offse, uint32_t siz, uint64_t uniqu, uint64_t nodei) 
  : in_header(sizeof(fuse_write_in) + siz, FUSE_WRITE, uniqu, nodei),
    write_in(f, offse, siz, 0, 0) {}
} virtio_fs_write_req;

typedef struct __attribute__((packed)) virtio_fs_write_res {
  fuse_out_header out_header;
  fuse_write_out write_out;
} virtio_fs_write_res;

/* CLOSE request and response */
typedef struct __attribute__((packed)) virtio_fs_close_req {
  fuse_in_header in_header;
  fuse_release_in release_in;

  virtio_fs_close_req(uint64_t f, uint32_t flag, uint32_t release_flag, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_release_in), FUSE_RELEASE, uniqu, nodei), 
    release_in(f, flag, release_flag) {}
} virtio_fs_close_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
} virtio_fs_close_res;

#endif // VIRTIO_FILESYSTEM_DEFS_HPP