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
 * @file plist_txt.h
 *
 * Handle the text marshalling and unmarshalling of plist objects in
 * the text format. A quick summary for the plist text format.
 *
 *   plist = dict / array / data / string
 *   dict = { "key" = "value"; ... }
 *   array = ( " ", ... )
 *   data = < hexadecimal codes in ASCII >
 *   string = "chars"
 *
 * The original ASCII representation is limited in that it
 * cannot represent an NSvalue (number, boolean) but this is handled
 * by adding extensions.
 *
 *   plist =/ date / value
 *   date = <*DYYYY-MM-DD HH:MM:SS timezone>
 *   value = true / false / number
 *
 * @version $Id: plist.h 11 2011-09-27 00:21:26Z ckhardin $
 */

#ifndef _PLIST_TXT_H_
#define _PLIST_TXT_H_

#include <plist.h>

/* forward declare */
typedef struct plist_txt_s plist_txt_t;

enum plist_txt_state_e {
	PLIST_TXT_STATE_ERROR = 0,
	PLIST_TXT_STATE_DONE,

	PLIST_TXT_STATE_SEARCH,
	PLIST_TXT_STATE_DICT_BEGIN,
	PLIST_TXT_STATE_DICT_KEY,
	PLIST_TXT_STATE_DICT_VALUE,
	PLIST_TXT_STATE_DICT_END,
	PLIST_TXT_STATE_ARRAY_BEGIN,
	PLIST_TXT_STATE_ARRAY_VALUE,
	PLIST_TXT_STATE_ARRAY_END,
	PLIST_TXT_STATE_DATA_BEGIN,
	PLIST_TXT_STATE_DATA_VALUE,
	PLIST_TXT_STATE_DATA_END,
	PLIST_TXT_STATE_STRING_BEGIN,
	PLIST_TXT_STATE_STRING_VALUE,
	PLIST_TXT_STATE_STRING_END,
	PLIST_TXT_STATE_DATE_BEGIN,
	PLIST_TXT_STATE_DATE_VALUE,
	PLIST_TXT_STATE_DATE_END,
	PLIST_TXT_STATE_TRUE_BEGIN,
	PLIST_TXT_STATE_TRUE_VALUE,
	PLIST_TXT_STATE_TRUE_END,
	PLIST_TXT_STATE_FALSE_BEGIN,
	PLIST_TXT_STATE_FALSE_VALUE,
	PLIST_TXT_STATE_FALSE_END,
	PLIST_TXT_STATE_NUMBER_BEGIN,
	PLIST_TXT_STATE_NUMBER_VALUE,
	PLIST_TXT_STATE_NUMBER_END
};

struct plist_txt_dict_s {
	const char *ptd_key;
};

struct plist_txt_true_s {
	int ptt_cnt;
	char ptt_buf[sizeof("true")];
};

struct plist_txt_false_s {
	int ptf_cnt;
};

struct plist_txt_number_s {
	int ptn_integer;
};
	

/**
 * Define the plist text parser context that is used to generate
 * a plist object representation from the specified input
 */
struct plist_txt_s {
	enum plist_txt_state_e pt_state;
	int pt_depth;
	plist_t *pt_top;
	plist_t *pt_cur;

	union {
		struct plist_txt_dict_s		ptu_dict;
		struct plist_txt_true_s		ptu_true;
		struct plist_txt_false_s	ptu_false;
		struct plist_txt_number_s	ptu_number;
	} pt_un;
#define pt_dict		pt_un.ptu_dict
#define pt_true		pt_un.ptu_true
#define pt_false	pt_un.ptu_false	
#define pt_number	pt_un.ptu_number
};

__BEGIN_DECLS

/**
 * Create a text parsing context that can be used for incremental
 * string parsing of the character objects
 *
 * @param  txtpp  result parse context for the plist
 * @return zero on success or an error value
 */
int plist_txt_new(plist_txt_t **txtpp);

/**
 * Free the text parsing context
 *
 * @param  txt  context that that was allocated with #plist_txt_new
 */
void plist_txt_free(plist_txt_t *txt);

/**
 * Parse a string fragment using the allocated context. This is an
 * incremental parse function that can process pieces of a string
 *
 * @param  txt  context that was allocated with #plist_txt_new
 * @param  str  pointer to a null terminated character array
 * @return zero on success or an error value
 */
int plist_txt_parse(plist_txt_t *txt, const char *str);

/**
 * Variant of the parse string fragment using the allocated context.
 * This is similar to #plist_txt_parse but does not require a
 * null terminated string.
 *
 * @param  txt  context that was allocated with #plist_txt_new
 * @param  str  pointer to a character array
 * @param  sz   size of the character array
 * @return zero on success or an error value
 */
int plist_txt_nparse(plist_txt_t *txt, const char *str, size_t sz);

__END_DECLS

#endif /* !_PLIST_TXT_H_ */
