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
 * @file plist.h
 *
 * Provide a C library for manipulating property list (plist) objects
 * and files. The plist is currently used in Apple Software and a fairly
 * elegant data structure type.
 *
 * @version $Revision$
 */

#ifndef _PLIST_H_
#define _PLIST_H_

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h> /* for struct tm */

/* forward declare */
typedef struct plist_s plist_t;
typedef struct plist_dict_s plist_dict_t;
typedef struct plist_key_s plist_key_t;
typedef struct plist_array_s plist_array_t;
typedef struct plist_data_s plist_data_t;
typedef struct plist_date_s plist_date_t;
typedef struct plist_string_s plist_string_t;
typedef struct plist_integer_s plist_integer_t;
typedef struct plist_real_s plist_real_t;
typedef struct plist_boolean_s plist_boolean_t;

/* Define the basic plist element types that are used to compose a plist
 * object.
 *
 * NOTE: These type are mapped to C99 standard types since there is no
 * Core Foundation Framework on all POSIX systems.
 */
enum plist_elem_e {
	PLIST_DICT,	/* dictionary of key value pairs */
	PLIST_KEY,	/* named type of an element in a dictionary */
	PLIST_ARRAY,	/* ordered array of plist elements */
	PLIST_DATA,	/* binary data */
	PLIST_DATE,	/* ISO 8601 calendar time */
	PLIST_STRING,	/* UTF-8 string */
	PLIST_INTEGER,	/* decimal number */
	PLIST_REAL,	/* floating point number */
	PLIST_BOOLEAN,	/* true or false */

	PLIST_UNKNOWN
};


struct plist_dict_s {
	int pd_numkeys;
	LIST_HEAD(, plist_s) pd_keys;
};

struct plist_key_s {
	char *pk_name;
	plist_t *pk_elem;
};

struct plist_array_s {
	int pa_numelems;
	LIST_HEAD(, plist_s) pa_elems;
};

struct plist_data_s {
	size_t pd_datasz;
	void *pd_data;
};

struct plist_date_s {
	struct tm pd_tm;
};

struct plist_string_s {
	char *ps_str;
};

struct plist_integer_s {
	int pi_int;
};

struct plist_real_s {
	double pr_double;
};

struct plist_boolean_s {
	bool pb_bool;
};

/* The plist structure is a discriminated union of all the plist elements */
struct plist_s {
	enum plist_elem_e p_elem;

	/* linkage for the tree (arrays and dictionaries) */
	struct plist_s *p_parent;
	LIST_ENTRY(plist_s) p_entry;

	union {
		struct plist_dict_s pu_dict;
		struct plist_key_s pu_key;
		struct plist_array_s pu_array;
		struct plist_data_s pu_data;
		struct plist_date_s pu_date;
		struct plist_string_s pu_string;
		struct plist_integer_s pu_integer;
		struct plist_real_s pu_real;
		struct plist_boolean_s pu_boolean;
	} p_un;
#define p_dict         p_un.pu_dict
#define p_key          p_un.pu_key
#define p_array        p_un.pu_array
#define p_data         p_un.pu_data
#define p_date         p_un.pu_date
#define p_string       p_un.pu_string
#define p_integer      p_un.pu_integer
#define p_real         p_un.pu_real
#define p_boolean      p_un.pu_boolean
};


/* iteration helper structure to loop thru elements of an array or a
 * dictionary
 */
struct plist_iterator_s {
	const plist_t *pi_elem;
	const void *pi_opaque;
};

__BEGIN_DECLS

/**
 * Convert a string to the plist element enumeration value.
 *
 * @param  str  string value of the plist element
 * @return enumeration value or a default PLIST_UNKNOWN
 */
enum plist_elem_e plist_stoe(const char *str);

/**
 * Return a constant string representation of the plist element value.
 *
 * @param  elem  plist enumeration value of the element
 * @return const string pointer or a default "unknown" string
 */
const char *plist_etos(enum plist_elem_e elem);


/*
 * Dictionaries and Keys
 */

/**
 * Initialize a plist dictionary element. This will allocate the required
 * memory and fill out the structure with default values.
 *
 * @param  dictpp  result plist dictionary element
 * @return zero on success or an error value
 */
int plist_dict_new(plist_t **dictpp);


/*
 * Arrays
 */

/**
 * Initialize a plist array element. This will allocate the required
 * memory and fill out the structure with default values.
 *
 * @param  arraypp  result plist array element
 * @return zero on success or an error value
 */
int plist_array_new(plist_t **arraypp);


/*
 * Simple Elements
 */

/**
 * Initialize a plist data element. This will allocate the required
 * memory and copy the passed in buffer to the plist element.
 *
 * @param  datapp  result plist data element
 * @param  buf     passed in data buffer
 * @param  bufsz   length of the data buffer
 * @return zero on success or an error value
 */
int plist_data_new(plist_t **datapp, const void *buf, size_t bufsz);

/**
 * Initialize a plist date element. This will allocate the required
 * memory and copy the broken down date into the plist element.
 *
 * @param  datepp  result plist date element
 * @param  tm      broken down calendar time
 * @return zero on success or an error value
 */
int plist_date_new(plist_t **datepp, const struct tm *tm);

/**
 * Initialize a plist string element. This will allocate the required
 * memory and copy the character string into the plist element.
 *
 * @param  stringpp  result plist date element
 * @param  s         null terminated character string
 * @return zero on success or an error value
 */
int plist_string_new(plist_t **stringpp, const char *s);

/**
 * Initialize a plist string element with a format. This will allocate the
 * required memory and copy the formatted string into the plist element.
 *
 * @param  stringpp  result plist date element
 * @param  fmt       formatted string specification
 * @return zero on success or an error value
 */
int plist_format_new(plist_t **stringpp, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

/**
 * Initialize a plist string element with a format and variable argument list.
 * This will allocate the required memory and copy the formatted string into
 * the plist element.
 *
 * @param  stringpp  result plist date element
 * @param  fmt       formatted string specification
 * @param  ap        variable argument list
 * @return zero on success or an error value
 */
int plist_vformat_new(plist_t **stringpp, const char *fmt, va_list ap)
	__attribute__ ((format (printf, 2, 0)));

/**
 * Initialize a plist integer element. This will allocate the required
 * memory and copy the decimal number into the plist element.
 *
 * @param  integerpp  result plist date element
 * @param  num        decimal number
 * @return zero on success or an error value
 */
int plist_integer_new(plist_t **integerpp, int num);

/**
 * Initialize a plist real element. This will allocate the required
 * memory and copy the real number into the plist element.
 *
 * @param  integerpp  result plist date element
 * @param  num        real number
 * @return zero on success or an error value
 */
int plist_real_new(plist_t **realpp, double num);

/**
 * Initialize a plist boolean element. This will allocate the required
 * memory and copy the boolean value into the plist elemet.
 *
 * @param  booleanpp  result plist boolean element
 * @param  flag       true or false
 * @return zero on success or an error value
 */
int plist_boolean_new(plist_t **booleanpp, bool flag);

/**
 * Deallocate a plist element and any children of the element. This is
 * a central routine to free any element type that has been allocated.
 *
 * @param  plist  element that should be freed
 */
void plist_free(plist_t *plist);

/**
 * Check if the plist is the specified element.
 *
 * @param  plist  element that should be checked
 * @param  type   enumeration value
 * @return true on a match, false in all other cases
 */
bool plist_iselem(const plist_t *plist, enum plist_elem_e type);

/**
 * Plist buffered io dump routine to pretty print a representation of
 * the property elements.
 *
 * @param  plist  reference to the element and children to print
 */
void plist_dump(const plist_t *plist, FILE *fp);

__END_DECLS

#endif /* !_PLIST_H_ */
