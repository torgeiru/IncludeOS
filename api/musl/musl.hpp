#pragma once
#ifndef MUSL_LIBC_HPP
#define MUSL_LIBC_HPP

#include <limits.h>
#include <vector>

/** Structures copied directly from MUSL 1.2.4 libc.
  * May need an updating when updating LIBC in the future.
  */
struct __locale_struct {
	const struct __locale_map *cat[6];
};

struct tls_module {
	struct tls_module *next;
	void *image;
	size_t len, size, align, offset;
};

struct __libc {
	char can_do_threads;
	char threaded;
	char secure;
	volatile signed char need_locks;
	int threads_minus_1;
	size_t *auxv;
	struct tls_module *tls_head;
	size_t tls_size, tls_align, tls_cnt;
	size_t page_size;
	struct __locale_struct global_locale;
};

/* TSD is the place where we store pointers to custom data blocks */
constexpr size_t PTHREAD_TSD_SIZE = PTHREAD_KEYS_MAX * sizeof(void*);

/* Custom type for vector used for handing over */
using thread_handover = std::vector<void*>;

#endif // MUSL_LIBC_HPP