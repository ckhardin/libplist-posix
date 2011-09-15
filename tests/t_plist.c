/*
 *
 * Copyright (c) 2011  Charles Hardin <ckhardin@gmail.com>
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file t_plist.c
 *
 * Test cases for the Property List POSIX library
 *
 * @version $Revision$
 */

#include <sys/types.h>
#include <sys/syslog.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <atf-c.h>

#include "plist.h"

ATF_TC(t_plist_dict);
ATF_TC_HEAD(t_plist_dict, tc)
{
	atf_tc_set_md_var(tc, "descr", "plist dictionary");
}

ATF_TC_BODY(t_plist_dict, tc)
{
	plist_t *dict;

	/* initialization */
	ATF_REQUIRE(plist_dict_new(&dict) == 0);

	plist_free(dict);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_plist_dict);
	return atf_no_error();
}
