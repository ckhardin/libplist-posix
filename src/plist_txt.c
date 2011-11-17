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
 * @file plist_txt.c
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "plist_txt.h"

int
plist_txt_new(plist_txt_t **txtpp)
{
	plist_txt_t *txt;

	if (!txtpp) {
		return EINVAL;
	}

	txt = malloc(sizeof(*txt));
	if (txt == NULL) {
		return ENOMEM;
	}
	memset(txt, 0, sizeof(*txt));

	txt->pt_state = PLIST_TXT_STATE_SEARCH;
	*txtpp = txt;
	return 0;
}


void
plist_txt_free(plist_txt_t *txt)
{
	if (!txt) {
		return;
	}

	if (txt->pt_top) {
		plist_free(txt->pt_top);
	}
	free(txt);
	return;
}


int
plist_txt_parse(plist_txt_t *txt, const char *str)
{
	if (!txt || !str) {
		return EINVAL;
	}
	return plist_txt_nparse(txt, str, strlen(str));
}


int
plist_txt_nparse(plist_txt_t *txt, const char *str, size_t sz)
{
	int err;
	off_t o;
	plist_t *ptmp;
	const char *c;

	if (!txt || !str) {
		return EINVAL;
	}
	if (sz == 0) {
		/* nothing to parse */
		return 0;
	}

	o = 0;
	c = str;
 nextstate:
	switch (txt->pt_state) {
	default:
	case PLIST_TXT_STATE_ERROR:
		return EACCES;

	case PLIST_TXT_STATE_DONE:
		return 0;

	case PLIST_TXT_STATE_SEARCH:
		/* eat whitespace */
		while (*c != '\0' && isblank(*c)) {
			c++; o++;
			if (o == sz) {
				return 0;
			}
		}
		switch (*c) {
		case '\0':
			return 0; /* hit a nil */
		case '{':
			txt->pt_state = PLIST_TXT_STATE_DICT_BEGIN;
			goto nextstate;
		case '(':
			txt->pt_state = PLIST_TXT_STATE_ARRAY_BEGIN;
			goto nextstate;
		case '<':
			txt->pt_state = PLIST_TXT_STATE_DATA_BEGIN;
			goto nextstate;
		case '"':
			txt->pt_state = PLIST_TXT_STATE_STRING_BEGIN;
			goto nextstate;
		case 'T':
		case 't':
			txt->pt_state = PLIST_TXT_STATE_TRUE_BEGIN;
			goto nextstate;
		case 'F':
		case 'f':
			txt->pt_state = PLIST_TXT_STATE_FALSE_BEGIN;
			goto nextstate;
		case '0' ... '9':
			txt->pt_state = PLIST_TXT_STATE_NUMBER_BEGIN;
			goto nextstate;
		default:
			break;
		}

		/* invalid character */
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return EINVAL;

	case PLIST_TXT_STATE_DICT_BEGIN:
		return ENOSYS;

	case PLIST_TXT_STATE_TRUE_BEGIN:
		txt->pt_true.ptt_buf[0] = tolower(*c);
		txt->pt_true.ptt_cnt = 1;

		/* fallthru */
		txt->pt_state = PLIST_TXT_STATE_TRUE_VALUE;
	case PLIST_TXT_STATE_TRUE_VALUE:
		while (txt->pt_true.ptt_cnt < strlen("true")) {
			c++; o++;
			if (o == sz) {
				return 0;
			}
			txt->pt_true.ptt_buf[txt->pt_true.ptt_cnt] = tolower(*c);
			txt->pt_true.ptt_cnt++;
		}
		if ((txt->pt_true.ptt_buf[0] != 't') ||
		    (txt->pt_true.ptt_buf[1] != 'r') ||
		    (txt->pt_true.ptt_buf[2] != 'u') ||
		    (txt->pt_true.ptt_buf[3] != 'e')) {
			/* invalid value */
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return EINVAL;
		}

		/* fallthru */
		txt->pt_state = PLIST_TXT_STATE_TRUE_END;
	case PLIST_TXT_STATE_TRUE_END:
		err = plist_boolean_new(&ptmp, true);
		if (err != 0) {
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return err;
		}
		if (txt->pt_depth == 0) {
			txt->pt_top = ptmp;
			txt->pt_state = PLIST_TXT_STATE_DONE;
			return 0;
		}

		/* insert into the current container */
		if (txt->pt_cur->p_elem == PLIST_KEY) {
			txt->pt_cur->p_key.pk_value = ptmp;
			ptmp->p_parent = txt->pt_cur;
			txt->pt_state = PLIST_TXT_STATE_DICT_END;
			goto nextstate;
		}
		if (txt->pt_cur->p_elem == PLIST_ARRAY) {
			TAILQ_INSERT_TAIL(&txt->pt_cur->p_array.pa_elems,
					  ptmp, p_entry);
			txt->pt_cur->p_array.pa_numelems++;
			txt->pt_state = PLIST_TXT_STATE_DICT_END;
			goto nextstate;
		}
		/* this really shouldn't be possible */
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return EINVAL;
	}

	return EAGAIN;
}
