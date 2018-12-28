/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "test.h"
__FBSDID("$FreeBSD: src/lib/libarchive/test/test_read_data_large.c,v 1.3 2007/05/29 01:00:20 kientzle Exp $");

/*
 * Test read/write of a 10M block of data in a single operation.
 * Uses an in-memory archive with a single 10M entry.  Exercises
 * archive_read_data() to ensure it can handle large blocks like
 * this and also exercises archive_read_data_into_fd() (which
 * had a bug relating to this, fixed in Nov 2006).
 */

char buff1[11000000];
char buff2[10000000];
char buff3[10000000];

DEFINE_TEST(test_read_data_large)
{
	struct archive_entry *ae;
	struct archive *a;
	char tmpfilename[] = "largefile";
	int tmpfilefd;
	unsigned int i;
	size_t used;

	/* Create a new archive in memory. */
	assert((a = archive_write_new()) != NULL);
	assertA(0 == archive_write_set_format_ustar(a));
	assertA(0 == archive_write_set_compression_none(a));
	assertA(0 == archive_write_open_memory(a, buff1, sizeof(buff1), &used));

	/*
	 * Write a file (with random contents) to it.
	 */
	assert((ae = archive_entry_new()) != NULL);
	archive_entry_copy_pathname(ae, "file");
	archive_entry_set_mode(ae, S_IFREG | 0755);
	for (i = 0; i < sizeof(buff2); i++)
		buff2[i] = (unsigned char)rand();
	archive_entry_set_size(ae, sizeof(buff2));
	assertA(0 == archive_write_header(a, ae));
	archive_entry_free(ae);
	assertA(sizeof(buff2) == archive_write_data(a, buff2, sizeof(buff2)));

	/* Close out the archive. */
	assertA(0 == archive_write_close(a));
#if ARCHIVE_API_VERSION > 1
	assertA(0 == archive_write_finish(a));
#else
	archive_write_finish(a);
#endif

	/* Check that archive_read_data can handle 10*10^6 at a pop. */
	assert((a = archive_read_new()) != NULL);
	assertA(0 == archive_read_support_format_all(a));
	assertA(0 == archive_read_support_compression_all(a));
	assertA(0 == archive_read_open_memory(a, buff1, sizeof(buff1)));
	assertA(0 == archive_read_next_header(a, &ae));
	failure("Wrote 10MB, but didn't read the same amount");
	assertEqualIntA(a, sizeof(buff2),archive_read_data(a, buff3, sizeof(buff3)));
	failure("Read expected 10MB, but data read didn't match what was written");
	assert(0 == memcmp(buff2, buff3, sizeof(buff3)));
	assert(0 == archive_read_close(a));
#if ARCHIVE_API_VERSION > 1
	assert(0 == archive_read_finish(a));
#else
	archive_read_finish(a);
#endif

	/* Check archive_read_data_into_fd */
	assert((a = archive_read_new()) != NULL);
	assertA(0 == archive_read_support_format_all(a));
	assertA(0 == archive_read_support_compression_all(a));
	assertA(0 == archive_read_open_memory(a, buff1, sizeof(buff1)));
	assertA(0 == archive_read_next_header(a, &ae));
	tmpfilefd = open(tmpfilename, O_WRONLY | O_CREAT | O_BINARY, 0777);
	assert(tmpfilefd != 0);
	assertEqualIntA(a, 0, archive_read_data_into_fd(a, tmpfilefd));
	assert(0 == archive_read_close(a));
#if ARCHIVE_API_VERSION > 1
	assert(0 == archive_read_finish(a));
#else
	archive_read_finish(a);
#endif
	close(tmpfilefd);

	tmpfilefd = open(tmpfilename, O_RDONLY | O_BINARY);
	assert(tmpfilefd != 0);
	assertEqualIntA(NULL, sizeof(buff3), read(tmpfilefd, buff3, sizeof(buff3)));
	close(tmpfilefd);
	assert(0 == memcmp(buff2, buff3, sizeof(buff3)));

	unlink(tmpfilename);
}
