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
 * @file plist.c
 *
 * Provide a C library for manipulating property list (plist) objects
 * and files. The plist is currently used in Apple Software and a fairly
 * elegant data structure type.
 *
 * @version $Id$
 */

#include <sys/types.h>
#include <sys/syslog.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "plist.h"


struct plist_nv_s {
	const char *pnv_name;
	enum plist_elem_e pnv_elem;
};

static struct plist_nv_s plist_nv_table[] = {
	{ "dict", PLIST_DICT },
	{ "key", PLIST_KEY },
	{ "array", PLIST_ARRAY },
	{ "data", PLIST_DATA },
	{ "date", PLIST_DATE },
	{ "string", PLIST_STRING },
	{ "integer", PLIST_INTEGER },
	{ "real", PLIST_REAL },
	{ "boolean", PLIST_BOOLEAN },

	{ NULL, PLIST_UNKNOWN }
};
#define N_PLISTNV  (sizeof(plist_nv_table)/sizeof(plist_nv_table[0]))


enum plist_elem_e
plist_stoe(const char *str)
{
	struct plist_nv_s *nv;

	if (!str) {
		return PLIST_UNKNOWN;
	}

	for (nv = plist_nv_table; nv->pnv_name != NULL; nv++) {
		if (strcasecmp(str, nv->pnv_name) == 0) {
			return nv->pnv_elem;
		}
	}
	return PLIST_UNKNOWN;
}


const char *
plist_etos(enum plist_elem_e elem)
{
	struct plist_nv_s *nv;

	for (nv = plist_nv_table; nv->pnv_name != NULL; nv++) {
		if (elem == nv->pnv_elem) {
			return nv->pnv_name;
		}
	}
	return "unknown";
}


/*
 * Dictionaries and Keys
 */
int
plist_dict_new(plist_t **dictpp)
{
	plist_t *dict;

	if (!dictpp) {
		return EINVAL;
	}

	dict = malloc(sizeof(*dict));
	if (dict == NULL) {
		return ENOMEM;
	}
	memset(dict, 0, sizeof(*dict));

	dict->p_elem = PLIST_DICT;
	LIST_INIT(&dict->p_dict.pd_keys);
	*dictpp = dict;
	return 0;
}


int
plist_dict_set(plist_t *dict, const char *name, plist_t *value)
{
	if (!dict || !name || !value) {
		return EINVAL;
	}
	return ENOSYS;
}


int
plist_dict_del(plist_t *dict, const char *name)
{
	if (!dict || !name) {
		return EINVAL;
	}
	return ENOSYS;
}


bool
plist_dict_haskey(const plist_t *dict, const char *name)
{
	if (!dict || !name) {
		return false;
	}
	return false;
}


int
plist_dict_update(plist_t *dict, const plist_t *other)
{
	if (!dict || !other) {
		return EINVAL;
	}
	return ENOSYS;
}


/*
 * Arrays
 */
int
plist_array_new(plist_t **arraypp)
{
	plist_t *array;

	if (!arraypp) {
		return EINVAL;
	}

	array = malloc(sizeof(*array));
	if (array == NULL) {
		return ENOMEM;
	}
	memset(array, 0, sizeof(*array));

	array->p_elem = PLIST_ARRAY;
	LIST_INIT(&array->p_array.pa_elems);
	*arraypp = array;
	return 0;
}


/*
 * Simple Elements
 */
int
plist_data_new(plist_t **datapp, const void *buf, size_t bufsz)
{
	plist_t *data;
	size_t sz;

	if (!datapp || !buf) {
		return EINVAL;
	}

	sz = sizeof(*data);
	sz += bufsz;
	data = malloc(sz);
	if (data == NULL) {
		return ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	data->p_elem = PLIST_DATA;
	data->p_data.pd_datasz = bufsz;
	data->p_data.pd_data = (uint8_t *) &data[1];
	memcpy(data->p_data.pd_data, buf, bufsz);
	*datapp = data;
	return 0;
}


int
plist_date_new(plist_t **datepp, const struct tm *tm)
{
	plist_t *date;

	if (!datepp || !tm) {
		return EINVAL;
	}

	date = malloc(sizeof(*date));
	if (date == NULL) {
		return ENOMEM;
	}
	memset(date, 0, sizeof(*date));

	date->p_elem = PLIST_DATE;
	memcpy(&date->p_date.pd_tm, tm, sizeof(*tm));
	*datepp = date;
	return 0;
}


int
plist_string_new(plist_t **stringpp, const char *s)
{
	plist_t *string;
	size_t sz;

	if (!stringpp || !s) {
		return EINVAL;
	}

	sz = sizeof(*string);
	sz += strlen(s) + 1;
	string = malloc(sz);
	if (string == NULL) {
		return ENOMEM;
	}
	memset(string, 0, sizeof(*string));

	string->p_elem = PLIST_STRING;
	string->p_string.ps_str = (char *) &string[1];
	memcpy(string->p_string.ps_str, s, strlen(s) + 1);
	*stringpp = string;
	return 0;
}


int
plist_format_new(plist_t **stringpp, const char *fmt, ...)
{
	int err;
	va_list ap;

	if (!stringpp || !fmt) {
		return EINVAL;
	}

	va_start(ap, fmt);
	err = plist_vformat_new(stringpp, fmt, ap);
	va_end(ap);
	return err;
}


int
plist_vformat_new(plist_t **stringpp, const char *fmt, va_list ap)
{
	plist_t *string;
	size_t sz;

	if (!stringpp || !fmt) {
		return EINVAL;
	}

	sz = sizeof(*string);
	sz += vsnprintf(NULL, 0, fmt, ap);
	string = malloc(sz);
	if (string == NULL) {
		return ENOMEM;
	}
	memset(string, 0, sizeof(*string));

	string->p_elem = PLIST_STRING;
	string->p_string.ps_str = (char *) &string[1];
	vsnprintf(string->p_string.ps_str, sz - sizeof(*string), fmt, ap);
	*stringpp = string;
	return 0;
}


int
plist_integer_new(plist_t **integerpp, int num)
{
	plist_t *integer;

	if (!integerpp) {
		return EINVAL;
	}

	integer = malloc(sizeof(*integer));
	if (integer == NULL) {
		return ENOMEM;
	}
	memset(integer, 0, sizeof(*integer));

	integer->p_elem = PLIST_INTEGER;
	integer->p_integer.pi_int = num;
	*integerpp = integer;
	return 0;
}


int
plist_real_new(plist_t **realpp, double num)
{
	plist_t *real;

	if (!realpp) {
		return EINVAL;
	}

	real = malloc(sizeof(*real));
	if (real == NULL) {
		return ENOMEM;
	}
	memset(real, 0, sizeof(*real));

	real->p_elem = PLIST_REAL;
	real->p_real.pr_double = num;
	*realpp = real;
	return 0;
}


int
plist_boolean_new(plist_t **booleanpp, bool flag)
{
	plist_t *boolean;

	if (!booleanpp) {
		return EINVAL;
	}

	boolean = malloc(sizeof(*boolean));
	if (boolean == NULL) {
		return ENOMEM;
	}
	memset(boolean, 0, sizeof(*boolean));

	boolean->p_elem = PLIST_BOOLEAN;
	boolean->p_boolean.pb_bool = flag;
	*booleanpp = boolean;
	return 0;
}


int
plist_copy(const plist_t *src, plist_t **dstpp)
{
	return ENOSYS;
}


void
plist_free(plist_t *plist)
{
	plist_t *ptmp;
	LIST_HEAD(, plist_s) pfree;

	if (!plist) {
		return;
	}

	ptmp = plist->p_parent;
	while (ptmp != NULL) {
		/* remove the parent reference */
		if (ptmp->p_elem == PLIST_DICT) {
			assert(ptmp->p_dict.pd_numkeys > 0);
			ptmp->p_dict.pd_numkeys--;
			LIST_REMOVE(plist, p_entry);
			break;
		}
		if (ptmp->p_elem == PLIST_ARRAY) {
			assert(ptmp->p_array.pa_numelems > 0);
			ptmp->p_array.pa_numelems--;
			LIST_REMOVE(plist, p_entry);
			break;
		}
		if (ptmp->p_elem == PLIST_KEY) {
			assert(ptmp->p_key.pk_elem == plist);
			ptmp->p_key.pk_elem = NULL;
			break;
		}
		/* something got broken since there shouldn't be any
		 * other parent types
		 */
		syslog(LOG_ERR, "plist free - invalid parent type %s for %p",
		       plist_etos(ptmp->p_elem), ptmp);
		return;
	}

	/* just use a list of things to free */
	LIST_INIT(&pfree);
	LIST_INSERT_HEAD(&pfree, plist, p_entry);

	while ((plist = LIST_FIRST(&pfree)) != NULL) {
		LIST_REMOVE(plist, p_entry);

		/* insert children of the current element into the free list */
		switch (plist->p_elem) {
		case PLIST_DICT:
			for (;;) {
				ptmp = LIST_FIRST(&plist->p_dict.pd_keys);
				if (ptmp == NULL) {
					break;
				}

				plist->p_dict.pd_numkeys--;
				LIST_REMOVE(ptmp, p_entry);
				LIST_INSERT_HEAD(&pfree, ptmp, p_entry);
			}
			assert(plist->p_dict.pd_numkeys == 0);
			break;
		case PLIST_KEY:
			if (plist->p_key.pk_elem != NULL) {
				ptmp = plist->p_key.pk_elem;
				LIST_INSERT_HEAD(&pfree, ptmp, p_entry);
				plist->p_key.pk_elem = NULL;
			}
			break;
		case PLIST_ARRAY:
			for (;;) {
				ptmp = LIST_FIRST(&plist->p_array.pa_elems);
				if (ptmp == NULL) {
					break;
				}

				plist->p_array.pa_numelems--;
				LIST_REMOVE(ptmp, p_entry);
				LIST_INSERT_HEAD(&pfree, ptmp, p_entry);
			}
			assert(plist->p_array.pa_numelems == 0);
			break;
		default:
			break;
		}

		/* every object is a single allocation */
		free(plist);
	}

	return;
}


bool
plist_iselem(const plist_t *plist, enum plist_elem_e elem)
{
	if (!plist) {
		return false;
	}
	return plist->p_elem == elem;
}


const plist_t *
plist_first(const plist_t *plist, plist_iterator_t *pi)
{
	const plist_t *pvar;

	if (!plist || !pi) {
		return NULL;
	}

	switch (plist->p_elem) {
	case PLIST_DICT:
		pvar = LIST_FIRST(&plist->p_dict.pd_keys);
		pi->pi_elem = plist->p_elem;
		pi->pi_opaque = pvar;
		return pvar;
	case PLIST_ARRAY:
		pvar = LIST_FIRST(&plist->p_array.pa_elems);
		pi->pi_elem = plist->p_elem;
		pi->pi_opaque = pvar;
		return pvar;
	default:
		break;
	}
	return NULL;
}


const plist_t *
plist_next(plist_iterator_t *pi)
{
	const plist_t *pvar;

	if (!pi || !pi->pi_opaque) {
		return NULL;
	}
	pvar = pi->pi_opaque;
	assert(pvar->p_parent != NULL);
	assert(pi->pi_elem == pvar->p_parent->p_elem);

	switch (pi->pi_elem) {
	case PLIST_DICT:
		pvar = LIST_NEXT(pvar, p_entry);
		pi->pi_opaque = pvar;
		return pvar;
	case PLIST_ARRAY:
		pvar = LIST_NEXT(pvar, p_entry);
		pi->pi_opaque = pvar;
		return pvar;
	default:
		break;
	}
	return NULL;
}


/**
 * Do a hex dump style of a buffer when a count, hex value, and then
 * printable characters at the end.
 */
static void
_datadump(FILE *fp, const void *buf, size_t bufsz)
{
#define _NUMELEM 16

	int i, j;
	int cnt;
	int pad;
	unsigned char ch;
	const unsigned char *ptr;

	ptr = buf;
	for (i = 0; i < bufsz; i += cnt) {
		fprintf(fp, "%u:\t", i);

		cnt = bufsz - i;
		if (cnt > _NUMELEM) {
			cnt = _NUMELEM;
		}
		for (j = 0; j < cnt; j++) {
			fprintf(fp, "%02x ", ptr[i + j]);
		}
		pad = (1 + _NUMELEM - j) * 3;
		fprintf(fp, "%*s", pad, "");

		/* now print the ASCII representation */
		for (j = 0; j < cnt; j++) {
			ch = ptr[i + j];
			if (!isprint(ch) || !isascii(ch)) {
				ch = '.';
			}
			fprintf(fp, "%c", ch);
		}
		fputs("\n", fp);
	}
	return;

#undef _NUMELEM
}

void
plist_dump(const plist_t *plist, FILE *fp)
{
#define _INDENTLEN  (8)

	int indent;
	const plist_t *pcur;
	const plist_t *pnext;
	char tmbuf[sizeof("YYYY-MM-DDThh:mm:ss.s+hh:mm")];

	if (!fp || !plist) {
		return;
	}

	indent = 0;
	for (pcur = plist; pcur; pcur = pnext) {
		pnext = NULL; /* default the next element */

		fprintf(fp, "%*s%s", indent * _INDENTLEN, "",
			plist_etos(pcur->p_elem));
		switch (pcur->p_elem) {
		case PLIST_DICT:
			fprintf(fp, "\n");
			pnext = LIST_FIRST(&pcur->p_dict.pd_keys);
			if (pnext != NULL) {
				indent++;
				continue;
			}
			break;
		case PLIST_KEY:
			fprintf(fp, "=%s\n", pcur->p_key.pk_name);
			pnext = pcur->p_key.pk_elem;
			if (pnext != NULL) {
				continue;
			}
			break;
		case PLIST_ARRAY:
			fprintf(fp, "\n");
			pnext = LIST_FIRST(&pcur->p_array.pa_elems);
			if (pnext != NULL) {
				indent++;
				continue;
			}
			break;
		case PLIST_DATA:
			fprintf(fp, "\n");
			_datadump(fp, pcur->p_data.pd_data,
				  pcur->p_data.pd_datasz);
			break;
		case PLIST_DATE:
			strftime(tmbuf, sizeof(tmbuf),
				 "%Y-%m-%dT%H:%M:%S%z",
				 &pcur->p_date.pd_tm);
			fprintf(fp, "=%s\n", tmbuf);
			break;
		case PLIST_STRING:
			fprintf(fp, "=%s\n", pcur->p_string.ps_str);
			break;
		case PLIST_INTEGER:
			fprintf(fp, "=%d\n", pcur->p_integer.pi_int);
			break;
		case PLIST_REAL:
			fprintf(fp, "=%f\n", pcur->p_real.pr_double);
			break;
		case PLIST_BOOLEAN:
			fprintf(fp, "=%s\n",
				pcur->p_boolean.pb_bool ? "true" : "false");
			break;
		default:
			break;
		}

		/* attempt to ascend */
		while (pcur != plist) {
			pnext = pcur->p_parent;
			if (pnext == NULL) {
				break;
			}

			if (pnext->p_elem == PLIST_DICT) {
				indent--;
				pcur = pnext;
				continue;
			}
			if (pnext->p_elem == PLIST_KEY) {
				pcur = pnext;
				pnext = LIST_NEXT(pcur, p_entry);
				if (pnext != NULL) {
					break;
				}
				continue;
			}
			if (pnext->p_elem == PLIST_ARRAY) {
				pnext = LIST_NEXT(pcur, p_entry);
				if (pnext != NULL) {
					break;
				}
				indent--;
				pcur = pcur->p_parent;
				continue;
			}

			/* data structure is messed up */
			pnext = NULL;
			break;
		}
	}

	return;

#undef _INDENTLEN
}
