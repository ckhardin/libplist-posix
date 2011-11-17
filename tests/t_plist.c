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
#include "plist_txt.h"


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


ATF_TC(t_plist_dict);
ATF_TC_HEAD(t_plist_dict, tc)
{
	atf_tc_set_md_var(tc, "descr", "plist dictionary");
}
ATF_TC_BODY(t_plist_dict, tc)
{
	struct tm tm;
	plist_t *dict;
	plist_t *ptmp;
	const char *type;
	const plist_t *pkey;
	plist_iterator_t piter;

	ATF_REQUIRE(plist_dict_new(&dict) == 0);

	/* insert each type into the dictionary */
	type = plist_etos(PLIST_DICT);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_dict_new(&ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_dict_new(&ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_ARRAY);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_array_new(&ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_array_new(&ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_DATA);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_data_new(&ptmp,
				   "DATAdata", sizeof("DATAdata")) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_data_new(&ptmp,
				   "DATAdata", sizeof("DATAdata")) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_DATE);
	memset(&tm, 0, sizeof(struct tm));
	strptime("1911-11-11 11:11:11", "%Y-%m-%d %H:%M:%S", &tm);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_date_new(&ptmp, &tm) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_date_new(&ptmp, &tm) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_STRING);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_string_new(&ptmp, "STRING") == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_string_new(&ptmp, "STRING") == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_INTEGER);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_integer_new(&ptmp, 1) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_integer_new(&ptmp, -1) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_REAL);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_real_new(&ptmp, 1.01) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_real_new(&ptmp, -1.01) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	type = plist_etos(PLIST_BOOLEAN);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_boolean_new(&ptmp, true) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == EPERM);
	ATF_REQUIRE(plist_boolean_new(&ptmp, false) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == true);

	PLIST_FOREACH(pkey, dict, &piter) {
		ATF_REQUIRE(pkey->p_elem == PLIST_KEY);
		ATF_REQUIRE_EQ(plist_stoe(pkey->p_key.pk_name),
			       pkey->p_key.pk_value->p_elem);
	}
	plist_dump(dict, stderr);

	/* test a remove and copy */
	type = plist_etos(PLIST_DICT);
	ATF_REQUIRE(plist_dict_del(dict, type) == 0);
	ATF_REQUIRE(plist_dict_haskey(dict, type) == false);
	ATF_REQUIRE(plist_copy(dict, &ptmp) == 0);
	ATF_REQUIRE(plist_dict_set(dict, type, ptmp) == 0);
	plist_dump(dict, stderr);

	plist_free(dict);
}


ATF_TC(t_plist_array);
ATF_TC_HEAD(t_plist_array, tc)
{
	atf_tc_set_md_var(tc, "descr", "plist array");
}
ATF_TC_BODY(t_plist_array, tc)
{
	struct tm tm;
	plist_t *array;
	plist_t *ptmp;
	const plist_t *pelem;
	plist_iterator_t piter;

	ATF_REQUIRE(plist_array_new(&array) == 0);

	/* insert each type into the array */
	ATF_REQUIRE(plist_dict_new(&ptmp) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	ATF_REQUIRE(plist_array_new(&ptmp) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	ATF_REQUIRE(plist_data_new(&ptmp, "ArrayData",
				   sizeof("ArrayData")) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	memset(&tm, 0, sizeof(struct tm));
	strptime("1912-12-12 12:12:12", "%Y-%m-%d %H:%M:%S", &tm);
	ATF_REQUIRE(plist_date_new(&ptmp, &tm) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	ATF_REQUIRE(plist_string_new(&ptmp, "ArrayString") == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	ATF_REQUIRE(plist_integer_new(&ptmp, INT32_MAX) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	ATF_REQUIRE(plist_real_new(&ptmp, 2.0202) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	ATF_REQUIRE(plist_boolean_new(&ptmp, true) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);

	PLIST_FOREACH(pelem, array, &piter) {
		plist_dump(pelem, stderr);
	}

	/* test a copy */
	ATF_REQUIRE(plist_copy(array, &ptmp) == 0);
	ATF_REQUIRE(plist_array_append(array, ptmp) == 0);
	plist_dump(array, stderr);

	plist_free(array);
}


ATF_TC(t_plist_txt);
ATF_TC_HEAD(t_plist_txt, tc)
{
	atf_tc_set_md_var(tc, "descr", "plist txt parser");
}
ATF_TC_BODY(t_plist_txt, tc)
{
	plist_t *ptmp;
	plist_txt_t *parse;

	ATF_REQUIRE(plist_txt_new(&parse) == 0);
	ATF_REQUIRE(plist_txt_parse(parse, "true", sizeof("true")) == 0);
	ATF_REQUIRE(plist_txt_result(parse, &ptmp) == 0);

	plist_dump(ptmp, stderr);
	
	plist_free(ptmp);
	plist_txt_free(parse);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_plist_new);
	ATF_TP_ADD_TC(tp, t_plist_dict);
	ATF_TP_ADD_TC(tp, t_plist_array);
	ATF_TP_ADD_TC(tp, t_plist_txt);
	return atf_no_error();
}
