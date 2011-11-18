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
	TAILQ_INIT(&dict->p_dict.pd_keys);
	*dictpp = dict;
	return 0;
}


int
plist_dict_set(plist_t *dict, const char *name, plist_t *value)
{
	size_t namesz;
	plist_t *key;
	plist_t *ptmp;

	if (!dict || !name || !value) {
		return EINVAL;
	}
	if (dict->p_elem != PLIST_DICT) {
		return EACCES;
	}
	if (value->p_parent != NULL) {
		return EPERM;
	}

	namesz = strlen(name) + 1;
	key = malloc(sizeof(*key) + namesz);
	if (key == NULL) {
		return ENOMEM;
	}
	memset(key, 0, sizeof(*key));

	/* attempt to delete an existing key here, since the allocation
	 * has already occurred for the new key
	 */
	TAILQ_FOREACH(ptmp, &dict->p_dict.pd_keys, p_entry) {
		if (strcmp(ptmp->p_key.pk_name, name) == 0) {
			break;
		}
	}
	if (ptmp != NULL) {
		plist_free(ptmp);
	}

	key->p_elem = PLIST_KEY;
	key->p_key.pk_name = (char *) &key[1];
	memcpy(key->p_key.pk_name, name, namesz);
	key->p_key.pk_value = value;
	value->p_parent = key;

	dict->p_dict.pd_numkeys++;
	TAILQ_INSERT_TAIL(&dict->p_dict.pd_keys, key, p_entry);
	key->p_parent = dict;
	return 0;
}


int
plist_dict_pop(plist_t *dict, const char *name, plist_t **plistpp)
{
	plist_t *ptmp;

	if (!dict || !name || !plistpp) {
		return EINVAL;
	}
	if (dict->p_elem != PLIST_DICT) {
		return EACCES;
	}

	TAILQ_FOREACH(ptmp, &dict->p_dict.pd_keys, p_entry) {
		if (strcmp(ptmp->p_key.pk_name, name) == 0) {
			break;
		}
	}
	if (ptmp == NULL) {
		return ENOENT;
	}

	dict->p_dict.pd_numkeys--;
	TAILQ_REMOVE(&dict->p_dict.pd_keys, ptmp, p_entry);
	ptmp->p_parent = NULL;
	*plistpp = ptmp;
	return 0;
}


int
plist_dict_del(plist_t *dict, const char *name)
{
	plist_t *ptmp;

	if (!dict || !name) {
		return EINVAL;
	}
	if (dict->p_elem != PLIST_DICT) {
		return EACCES;
	}

	TAILQ_FOREACH(ptmp, &dict->p_dict.pd_keys, p_entry) {
		if (strcmp(ptmp->p_key.pk_name, name) == 0) {
			break;
		}
	}
	if (ptmp != NULL) {
		plist_free(ptmp);
	}
	return 0;
}


bool
plist_dict_haskey(const plist_t *dict, const char *name)
{
	plist_t *ptmp;

	if (!dict || !name) {
		return false;
	}
	if (dict->p_elem != PLIST_DICT) {
		return EACCES;
	}

	TAILQ_FOREACH(ptmp, &dict->p_dict.pd_keys, p_entry) {
		if (strcmp(ptmp->p_key.pk_name, name) == 0) {
			return true;
		}
	}
	return false;
}


int
plist_dict_update(plist_t *dict, const plist_t *other)
{
	int err;
	plist_t *ptmp;
	plist_t *pcopy;
	TAILQ_HEAD(, plist_s) plist;

	if (!dict || !other) {
		return EINVAL;
	}
	if (dict->p_elem != PLIST_DICT) {
		return EACCES;
	}

	TAILQ_INIT(&plist);
	if (other->p_elem == PLIST_DICT) {
		TAILQ_FOREACH(ptmp, &other->p_dict.pd_keys, p_entry) {
			err = plist_copy(ptmp, &pcopy);
			if (err != 0) {
				goto bail;
			}
			TAILQ_INSERT_TAIL(&plist, pcopy, p_entry);
		}

		while ((pcopy = TAILQ_FIRST(&plist)) != NULL) {
			TAILQ_REMOVE(&plist, pcopy, p_entry);

			plist_dict_del(dict, pcopy->p_key.pk_name);

			dict->p_dict.pd_numkeys++;
			TAILQ_INSERT_TAIL(&dict->p_dict.pd_keys,
					  pcopy, p_entry);
		}
		return 0;
	}
	if (other->p_elem == PLIST_KEY) {
		err = plist_copy(other, &pcopy);
		if (err != 0) {
			goto bail;
		}

		plist_dict_del(dict, pcopy->p_key.pk_name);

		dict->p_dict.pd_numkeys++;
		TAILQ_INSERT_TAIL(&dict->p_dict.pd_keys, pcopy, p_entry);
		return 0;
	}
	if (other->p_elem == PLIST_ARRAY) {
		TAILQ_FOREACH(ptmp, &other->p_array.pa_elems, p_entry) {
			if (ptmp->p_elem != PLIST_KEY) {
				/* can only copy array of keys */
				err = EPERM;
				goto bail;
			}

			err = plist_copy(ptmp, &pcopy);
			if (err != 0) {
				goto bail;
			}
			TAILQ_INSERT_TAIL(&plist, pcopy, p_entry);
		}

		while ((pcopy = TAILQ_FIRST(&plist)) != NULL) {
			TAILQ_REMOVE(&plist, pcopy, p_entry);

			plist_dict_del(dict, pcopy->p_key.pk_name);

			dict->p_dict.pd_numkeys++;
			TAILQ_INSERT_TAIL(&dict->p_dict.pd_keys,
					  pcopy, p_entry);
		}
		return 0;
	}

	err = EPERM;
 bail:
	while ((ptmp = TAILQ_FIRST(&plist)) != NULL) {
		TAILQ_REMOVE(&plist, ptmp, p_entry);
		plist_free(ptmp);
	}
	return err;
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
	TAILQ_INIT(&array->p_array.pa_elems);
	*arraypp = array;
	return 0;
}


int
plist_array_append(plist_t *array, plist_t *value)
{
	if (!array || !value) {
		return EINVAL;
	}
	if (array->p_elem != PLIST_ARRAY) {
		return EACCES;
	}
	if (value->p_parent != NULL) {
		return EPERM;
	}

	array->p_array.pa_numelems++;
	TAILQ_INSERT_TAIL(&array->p_array.pa_elems, value, p_entry);
	value->p_parent = array;
	return 0;
}


int
plist_array_insert(plist_t *array, int loc, plist_t *value)
{
	int i;
	plist_t *ptmp;

	if (!array || !value) {
		return EINVAL;
	}
	if (array->p_elem != PLIST_ARRAY) {
		return EACCES;
	}
	if ((loc < 0) || (loc > array->p_array.pa_numelems)) {
		return ERANGE;
	}
	if (value->p_parent != NULL) {
		return EPERM;
	}

	/* special case the tail */
	if (loc == array->p_array.pa_numelems) {
		array->p_array.pa_numelems++;
		TAILQ_INSERT_TAIL(&array->p_array.pa_elems, value, p_entry);
		value->p_parent = array;
		return 0;
	}

	i = 0;
	TAILQ_FOREACH(ptmp, &array->p_array.pa_elems, p_entry) {
		if (i == loc) {
			break;
		}
		i++;
	}
	assert(ptmp != NULL);
	
	array->p_array.pa_numelems++;
	TAILQ_INSERT_BEFORE(ptmp, value, p_entry);
	value->p_parent = array;
	return 0;
}


int
plist_array_pop(plist_t *array, int loc, plist_t **plistpp)
{
	int i;
	plist_t *ptmp;

	if (!array || !plistpp) {
		return EINVAL;
	}
	if (array->p_elem != PLIST_ARRAY) {
		return EACCES;
	}
	if ((loc < 0) || (loc >= array->p_array.pa_numelems)) {
		return ERANGE;
	}

	i = 0;
	TAILQ_FOREACH(ptmp, &array->p_array.pa_elems, p_entry) {
		if (i == loc) {
			break;
		}
		i++;
	}
	assert(ptmp != NULL);
	
	array->p_array.pa_numelems--;
	TAILQ_REMOVE(&array->p_array.pa_elems, ptmp, p_entry);
	ptmp->p_parent = NULL;
	*plistpp = ptmp;
	return 0;
}


int
plist_array_del(plist_t *array, int loc)
{
	int i;
	plist_t *ptmp;

	if (!array) {
		return EINVAL;
	}
	if (array->p_elem != PLIST_ARRAY) {
		return EACCES;
	}
	if ((loc < 0) || (loc >= array->p_array.pa_numelems)) {
		return ERANGE;
	}

	i = 0;
	TAILQ_FOREACH(ptmp, &array->p_array.pa_elems, p_entry) {
		if (i == loc) {
			break;
		}
		i++;
	}
	assert(ptmp != NULL);

	plist_free(ptmp);
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
	size_t sz;
	va_list apcopy;
	char scratch[1];
	plist_t *string;

	if (!stringpp || !fmt) {
		return EINVAL;
	}

	sz = sizeof(*string);
	va_copy(apcopy, ap);
	sz += vsnprintf(scratch, sizeof(scratch), fmt, apcopy) + 1;
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


/**
 * Helper function to copy one element for the copy tree decent.
 */
static int
_plist_copyelem(const plist_t *s, plist_t **d)
{
	int err;
	const plist_t *stmp;
	plist_t *dtmp;

	stmp = s;
	if (s->p_elem == PLIST_KEY) {
		/* keys copy the value first and then the key */
		stmp = s->p_key.pk_value;
	}

	switch (stmp->p_elem) {
	case PLIST_DICT:
		err = plist_dict_new(&dtmp);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_ARRAY:
		err = plist_array_new(&dtmp);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_DATA:
		err = plist_data_new(&dtmp, stmp->p_data.pd_data,
				     stmp->p_data.pd_datasz);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_DATE:
		err = plist_date_new(&dtmp, &stmp->p_date.pd_tm);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_STRING:
		err = plist_string_new(&dtmp, stmp->p_string.ps_str);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_INTEGER:
		err = plist_integer_new(&dtmp, stmp->p_integer.pi_int);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_REAL:
		err = plist_real_new(&dtmp, stmp->p_real.pr_double);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_BOOLEAN:
		err = plist_boolean_new(&dtmp, stmp->p_boolean.pb_bool);
		if (err != 0) {
			goto bail;
		}
		break;
	case PLIST_KEY:
		/* should not have a key type - see above */
	default:
		return EACCES;
	}

	if (s->p_elem == PLIST_KEY) {
		plist_t *key;
		size_t namesz;

		namesz = strlen(s->p_key.pk_name) + 1;
		key = malloc(sizeof(*key) + namesz);
		if (key == NULL) {
			err = ENOMEM;
			plist_free(dtmp);
			goto bail;
		}
		memset(key, 0, sizeof(*key));

		key->p_elem = PLIST_KEY;
		key->p_key.pk_name = (char *) &key[1];
		memcpy(key->p_key.pk_name, s->p_key.pk_name, namesz);
		key->p_key.pk_value = dtmp;
		dtmp->p_parent = key;
		dtmp = key;
	}

	err = 0;
	*d = dtmp;
 bail:
	return err;
}


int
plist_copy(const plist_t *src, plist_t **dstpp)
{
	int err;
	plist_t *dst;
	plist_t *pcopycur;
	plist_t *pcopyprev;
	const plist_t *pcur;
	const plist_t *pnext;

	if (!src || !dstpp) {
		return EINVAL;
	}

	dst = NULL;
	pcopycur = NULL;
	pcopyprev = NULL;
	for (pcur = src; pcur; pcur = pnext) {
		pnext = NULL; /* default the next element */

		err = _plist_copyelem(pcur, &pcopycur);
		if (err != 0) {
			goto bail;
		}
		if (dst == NULL) {
			dst = pcopycur;
		}

		if (pcopyprev != NULL) {
			switch (pcopyprev->p_elem) {
			case PLIST_DICT:
				pcopycur->p_parent = pcopyprev;
				pcopyprev->p_dict.pd_numkeys++;
				TAILQ_INSERT_TAIL(&pcopyprev->p_dict.pd_keys,
						 pcopycur, p_entry);
				break;
			case PLIST_KEY:
				pcopycur->p_parent = pcopyprev->p_parent;
				pcopyprev->p_parent->p_dict.pd_numkeys++;
				TAILQ_INSERT_TAIL(
					&pcopyprev->p_parent->p_dict.pd_keys,
					pcopycur, p_entry);
				break;
			case PLIST_ARRAY:
				pcopycur->p_parent = pcopyprev;
				pcopyprev->p_array.pa_numelems++;
				TAILQ_INSERT_TAIL(&pcopyprev->p_array.pa_elems,
						 pcopycur, p_entry);
				break;
			default:
				break;
			}
		}

		/* setup the next copy */
		pnext = pcur;
		pcopyprev = pcopycur;
		if (pnext->p_elem == PLIST_KEY) {
			pnext = pnext->p_key.pk_value;
			pcopyprev = pcopyprev->p_key.pk_value;

			/* adjust the current pointer thru the key */
			pcur = pcur->p_key.pk_value;
		}

		switch (pnext->p_elem) {
		case PLIST_DICT:
			pnext = TAILQ_FIRST(&pnext->p_dict.pd_keys);
			if (pnext != NULL) {
				continue;
			}
			break;
		case PLIST_ARRAY:
			pnext = TAILQ_FIRST(&pnext->p_array.pa_elems);
			if (pnext != NULL) {
				continue;
			}
			break;
		default:
			break;
		}

		/* attempt to ascend */
		for (;;) {
			if (pcur == src || pcur->p_parent == NULL) {
				pnext = NULL;
				break;
			}
			pnext = pcur->p_parent;

			if (pnext->p_elem == PLIST_DICT) {
				pcopyprev = pcopyprev->p_parent;
				pcur = pnext;
				continue;
			}
			if (pnext->p_elem == PLIST_KEY) {
				pcopyprev = pcopyprev->p_parent;
				pnext = TAILQ_NEXT(pnext, p_entry);
				if (pnext != NULL) {
					break;
				}
				pcur = pcur->p_parent;
				continue;
			}
			if (pnext->p_elem == PLIST_ARRAY) {
				pcopyprev = pcopyprev->p_parent;
				pnext = TAILQ_NEXT(pcur, p_entry);
				if (pnext != NULL) {
					break;
				}
				pcur = pcur->p_parent;
				continue;
			}

			/* data structure is messed up */
			pnext = NULL;
			break;
		}
	}

	*dstpp = dst;
	return 0;

 bail:
	if (dst != NULL) {
		plist_free(dst);
	}
	return err;
}


void
plist_free(plist_t *plist)
{
	plist_t *ptmp;
	TAILQ_HEAD(, plist_s) pfree;

	if (!plist) {
		return;
	}

	ptmp = plist->p_parent;
	while (ptmp != NULL) {
		/* remove the parent reference */
		if (ptmp->p_elem == PLIST_DICT) {
			assert(ptmp->p_dict.pd_numkeys > 0);
			ptmp->p_dict.pd_numkeys--;
			TAILQ_REMOVE(&ptmp->p_dict.pd_keys, plist, p_entry);
			break;
		}
		if (ptmp->p_elem == PLIST_ARRAY) {
			assert(ptmp->p_array.pa_numelems > 0);
			ptmp->p_array.pa_numelems--;
			TAILQ_REMOVE(&ptmp->p_array.pa_elems, plist, p_entry);
			break;
		}
		if (ptmp->p_elem == PLIST_KEY) {
			assert(ptmp->p_key.pk_value == plist);
			ptmp->p_key.pk_value = NULL;
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
	TAILQ_INIT(&pfree);
	TAILQ_INSERT_TAIL(&pfree, plist, p_entry);

	while ((plist = TAILQ_FIRST(&pfree)) != NULL) {
		TAILQ_REMOVE(&pfree, plist, p_entry);

		/* insert children of the current element into the free list */
		switch (plist->p_elem) {
		case PLIST_DICT:
			for (;;) {
				ptmp = TAILQ_FIRST(&plist->p_dict.pd_keys);
				if (ptmp == NULL) {
					break;
				}

				plist->p_dict.pd_numkeys--;
				TAILQ_REMOVE(&plist->p_dict.pd_keys,
					     ptmp, p_entry);
				TAILQ_INSERT_TAIL(&pfree, ptmp, p_entry);
			}
			assert(plist->p_dict.pd_numkeys == 0);
			break;
		case PLIST_KEY:
			if (plist->p_key.pk_value != NULL) {
				ptmp = plist->p_key.pk_value;
				TAILQ_INSERT_TAIL(&pfree, ptmp, p_entry);
				plist->p_key.pk_value = NULL;
			}
			break;
		case PLIST_ARRAY:
			for (;;) {
				ptmp = TAILQ_FIRST(&plist->p_array.pa_elems);
				if (ptmp == NULL) {
					break;
				}

				plist->p_array.pa_numelems--;
				TAILQ_REMOVE(&plist->p_array.pa_elems,
					     ptmp, p_entry);
				TAILQ_INSERT_TAIL(&pfree, ptmp, p_entry);
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


plist_t *
plist_first(const plist_t *plist, plist_iterator_t *pi)
{
	plist_t *pvar;

	if (!plist || !pi) {
		return NULL;
	}

	switch (plist->p_elem) {
	case PLIST_DICT:
		pvar = TAILQ_FIRST(&plist->p_dict.pd_keys);
		pi->pi_elem = plist->p_elem;
		pi->pi_opaque =
		    (pvar == NULL) ? NULL : TAILQ_NEXT(pvar, p_entry);
		return pvar;
	case PLIST_ARRAY:
		pvar = TAILQ_FIRST(&plist->p_array.pa_elems);
		pi->pi_elem = plist->p_elem;
		pi->pi_opaque =
		    (pvar == NULL) ? NULL : TAILQ_NEXT(pvar, p_entry);
		return pvar;
	default:
		break;
	}
	return NULL;
}


plist_t *
plist_next(plist_iterator_t *pi)
{
	plist_t *pvar;

	if (!pi || !pi->pi_opaque) {
		return NULL;
	}
	pvar = pi->pi_opaque;
	assert(pvar->p_parent != NULL);
	assert(pi->pi_elem == pvar->p_parent->p_elem);

	switch (pi->pi_elem) {
	case PLIST_DICT:
		pi->pi_opaque =
		    (pvar == NULL) ? NULL : TAILQ_NEXT(pvar, p_entry);
		return pvar;
	case PLIST_ARRAY:
		pi->pi_opaque =
		    (pvar == NULL) ? NULL : TAILQ_NEXT(pvar, p_entry);
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

	if (!plist || !fp) {
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
			pnext = TAILQ_FIRST(&pcur->p_dict.pd_keys);
			if (pnext != NULL) {
				indent++;
				continue;
			}
			break;
		case PLIST_KEY:
			fprintf(fp, "=%s\n", pcur->p_key.pk_name);
			pnext = pcur->p_key.pk_value;
			if (pnext != NULL) {
				continue;
			}
			break;
		case PLIST_ARRAY:
			fprintf(fp, "\n");
			pnext = TAILQ_FIRST(&pcur->p_array.pa_elems);
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
		for (;;) {
			if (pcur == plist || pcur->p_parent == NULL) {
				pnext = NULL;
				break;
			}
			pnext = pcur->p_parent;

			if (pnext->p_elem == PLIST_DICT) {
				indent--;
				pcur = pnext;
				continue;
			}
			if (pnext->p_elem == PLIST_KEY) {
				pcur = pnext;
				pnext = TAILQ_NEXT(pcur, p_entry);
				if (pnext != NULL) {
					break;
				}
				continue;
			}
			if (pnext->p_elem == PLIST_ARRAY) {
				pnext = TAILQ_NEXT(pcur, p_entry);
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
