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

ATF_TC(t_plist_new);
ATF_TC_HEAD(t_plist_new, tc)
{
	atf_tc_set_md_var(tc, "descr", "plist new methods");
}

ATF_TC_BODY(t_plist_new, tc)
{
	plist_t *ptmp;
	struct tm tm;

	/* initialization */
	ATF_REQUIRE(plist_dict_new(&ptmp) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_DICT) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("dict")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	ATF_REQUIRE(plist_array_new(&ptmp) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_ARRAY) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("array")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	ATF_REQUIRE(plist_data_new(&ptmp, "databuffer",
				   sizeof("databuffer")) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_DATA) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("data")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	memset(&tm, 0, sizeof(struct tm));
	strptime("2001-11-12 18:31:01", "%Y-%m-%d %H:%M:%S", &tm);
	ATF_REQUIRE(plist_date_new(&ptmp, &tm) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_DATE) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("date")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	ATF_REQUIRE(plist_string_new(&ptmp, "string") == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_STRING) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("string")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);
	ATF_REQUIRE(plist_format_new(&ptmp, "%s%c%s",
				     "format", '-', "string") == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_STRING) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("string")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	ATF_REQUIRE(plist_integer_new(&ptmp, -1) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_INTEGER) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("integer")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	ATF_REQUIRE(plist_real_new(&ptmp, 0.123) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_REAL) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("real")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);

	ATF_REQUIRE(plist_boolean_new(&ptmp, true) == 0);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_BOOLEAN) == true);
	ATF_REQUIRE(plist_iselem(ptmp, plist_stoe("boolean")) == true);
	ATF_REQUIRE(plist_iselem(ptmp, PLIST_UNKNOWN) == false);
	plist_free(ptmp);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_plist_new);
	return atf_no_error();
}
