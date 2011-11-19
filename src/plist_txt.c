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
#include <assert.h>

#include "plist_txt.h"


/* forward declare */
typedef struct plist_chunk_s plist_chunk_t;

struct plist_chunk_s {
	const char *pc_cp; /* current */
	const char *pc_ep; /* end pointer */
};


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

	txt->pt_state = PLIST_TXT_STATE_SCAN;
	*txtpp = txt;
	return 0;
}


void
plist_txt_free(plist_txt_t *txt)
{
	if (!txt) {
		return;
	}

	if (txt->pt_top != NULL) {
		plist_free(txt->pt_top);
	}
	if (txt->pt_buf != NULL) {
		free(txt->pt_buf);
	}
	free(txt);
	return;
}


static void
_plist_txt_push(plist_txt_t *txt, plist_t *value)
{
	if (txt->pt_depth == 0) {
		txt->pt_top = value;
		txt->pt_cur = value;
		txt->pt_depth++;
		txt->pt_state = PLIST_TXT_STATE_SCAN;
		return;
	}
	if (txt->pt_cur == NULL) {
		plist_free(value);
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}

	switch (txt->pt_cur->p_elem) {
	case PLIST_KEY:
		plist_dict_set(txt->pt_cur->p_parent,
			       txt->pt_cur->p_key.pk_name, value);
		break;
	case PLIST_ARRAY:
		plist_array_append(txt->pt_cur, value);
		break;
	default:
		/* invalid parent */
		plist_free(value);
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}
	txt->pt_cur = value;
	txt->pt_depth++;
	txt->pt_state = PLIST_TXT_STATE_SCAN;
	return;
}

static void
_plist_txt_next(plist_txt_t *txt, plist_t *value)
{
	if (txt->pt_depth == 0) {
		txt->pt_top = value;
		txt->pt_cur = value;
		txt->pt_state = PLIST_TXT_STATE_SCAN;
		return;
	}
	if (txt->pt_cur == NULL) {
		plist_free(value);
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}

	/* insert into the current container */
	switch (txt->pt_cur->p_elem) {
	case PLIST_KEY:
		plist_dict_set(txt->pt_cur->p_parent,
			       txt->pt_cur->p_key.pk_name, value);
		break;
	case PLIST_ARRAY:
		plist_array_append(txt->pt_cur, value);
		break;
	default:
		/* invalid parent */
		plist_free(value);
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}
	txt->pt_state = PLIST_TXT_STATE_SCAN;
	return;
}

static void
_plist_txt_pop(plist_txt_t *txt)
{
	if (txt->pt_depth <= 0 || txt->pt_cur == NULL) {
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}
	txt->pt_depth--;
	txt->pt_cur = txt->pt_cur->p_parent;
	txt->pt_state = PLIST_TXT_STATE_SCAN;
	return;
}

static int
_plist_txt_buf(plist_txt_t *txt, size_t extend)
{
	void *ptr;

	if ((txt->pt_bufsz - txt->pt_bufoff) >= extend) {
		return 0;
	}

	ptr = realloc(txt->pt_buf, txt->pt_bufsz + extend);
	if (ptr == NULL) {
		return ENOMEM;
	}
	txt->pt_buf = ptr;
	txt->pt_bufsz += extend;
	return 0;
}

static void
_plist_txt_true(plist_txt_t *txt, plist_chunk_t *chunk)
{
	int err;
	char *bp;
	plist_t *ptmp;

	/* pickup the fragment in the buffer */
	bp = txt->pt_buf;
	while (txt->pt_bufoff < strlen("true")) {
		if (chunk->pc_cp == chunk->pc_ep) {
			txt->pt_state = PLIST_TXT_STATE_TRUE;
			return;
		}
		bp[txt->pt_bufoff] = chunk->pc_cp[0];
		txt->pt_bufoff++;
		chunk->pc_cp++;
	}
	if ((tolower(bp[0]) != 't') ||
	    (tolower(bp[1]) != 'r') ||
	    (tolower(bp[2]) != 'u') ||
	    (tolower(bp[3]) != 'e')) {
		/* invalid value */
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}

	err = plist_boolean_new(&ptmp, true);
	if (err != 0) {
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}
	_plist_txt_next(txt, ptmp);
	return;
}

static void
_plist_txt_false(plist_txt_t *txt, plist_chunk_t *chunk)
{
	int err;
	char *bp;
	plist_t *ptmp;

	/* pickup the fragment in the buffer */
	bp = txt->pt_buf;
	while (txt->pt_bufoff < strlen("false")) {
		if (chunk->pc_cp == chunk->pc_ep) {
			txt->pt_state = PLIST_TXT_STATE_FALSE;
			return;
		}
		bp[txt->pt_bufoff] = chunk->pc_cp[0];
		txt->pt_bufoff++;
		chunk->pc_cp++;
	}
	if ((tolower(bp[0]) != 'f') ||
	    (tolower(bp[1]) != 'a') ||
	    (tolower(bp[2]) != 'l') ||
	    (tolower(bp[3]) != 's') ||
	    (tolower(bp[4]) != 'e')) {
		/* invalid value */
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}

	err = plist_boolean_new(&ptmp, false);
	if (err != 0) {
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return;
	}
	_plist_txt_next(txt, ptmp);
	return;
}


int
plist_txt_parse(plist_txt_t *txt, const void *buf, size_t sz)
{
	int err;
	plist_t *ptmp;
	plist_chunk_t chunk;

	if (!txt || !buf) {
		return EINVAL;
	}
	if (sz == 0) {
		/* nothing to parse */
		return 0;
	}

	chunk.pc_cp = buf; /* current */
	chunk.pc_ep = &chunk.pc_cp[sz]; /* end */
 nextstate:
	switch (txt->pt_state) {
	case PLIST_TXT_STATE_DONE:
		return 0;

	case PLIST_TXT_STATE_SCAN:
		/* eat whitespace */
		while (chunk.pc_cp[0] != '\0' && isblank(chunk.pc_cp[0])) {
			if (chunk.pc_cp == chunk.pc_ep) {
				break;
			}
			chunk.pc_cp++;
		}

		switch (chunk.pc_cp[0]) {
		case '{':
			err = plist_dict_new(&ptmp);
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_push(txt, ptmp);

			chunk.pc_cp++;
			goto nextstate;

		case '}':
			if (txt->pt_cur == NULL ||
			    txt->pt_cur->p_elem != PLIST_DICT) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EACCES;
			}
			_plist_txt_pop(txt);

			chunk.pc_cp++;
			goto nextstate;

		case '(':
			err = plist_array_new(&ptmp);
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_push(txt, ptmp);

			chunk.pc_cp++;
			goto nextstate;

		case ')':
			if (txt->pt_cur == NULL ||
			    txt->pt_cur->p_elem != PLIST_ARRAY) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EACCES;
			}
			_plist_txt_pop(txt);

			chunk.pc_cp++;
			goto nextstate;

		case ',':
			/* just the next element in an array */
			if (txt->pt_cur == NULL ||
			    txt->pt_cur->p_elem != PLIST_ARRAY) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EACCES;
			}

			chunk.pc_cp++;
			goto nextstate;

		case '<':
			txt->pt_state = PLIST_TXT_STATE_DATA_BEGIN;
			goto nextstate;

		case '"':
			/* eat until we get another '"' */
			txt->pt_state = PLIST_TXT_STATE_STRING_BEGIN;
			goto nextstate;

		case 'T':
		case 't':
			txt->pt_bufoff = 0;
			err = _plist_txt_buf(txt, sizeof("true"));
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_true(txt, &chunk);
			if (chunk.pc_cp != chunk.pc_ep) {
				chunk.pc_cp++;
				goto nextstate;
			}
			break;

		case 'F':
		case 'f':
			txt->pt_bufoff = 0;
			err = _plist_txt_buf(txt, sizeof("false"));
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_false(txt, &chunk);
			if (chunk.pc_cp != chunk.pc_ep) {
				chunk.pc_cp++;
				goto nextstate;
			}
			break;

		case '0' ... '9':
			txt->pt_state = PLIST_TXT_STATE_NUMBER_BEGIN;
			goto nextstate;

		default:
			break;
		}

		if (txt->pt_top != NULL && txt->pt_depth == 0 &&
		    chunk.pc_cp == chunk.pc_ep) {
			txt->pt_state = PLIST_TXT_STATE_DONE;
			return 0;
		}

		/* invalid character */
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return EINVAL;

	case PLIST_TXT_STATE_TRUE:
		assert(strlen("true") < txt->pt_bufsz);
		assert(txt->pt_bufoff < txt->pt_bufsz);

		_plist_txt_true(txt, &chunk);
		goto nextstate;

	case PLIST_TXT_STATE_FALSE:
		assert(strlen("false") < txt->pt_bufsz);
		assert(txt->pt_bufoff < txt->pt_bufsz);

		_plist_txt_false(txt, &chunk);
		goto nextstate;

	default:
	case PLIST_TXT_STATE_ERROR:
		return EACCES;
	}

	return EAGAIN;
}


int
plist_txt_result(plist_txt_t *txt, plist_t **plistpp)
{
	plist_t *ptmp;
	enum plist_txt_state_e pstate;

	if (!txt || !plistpp) {
		return EINVAL;
	}
	ptmp = txt->pt_top;
	pstate = txt->pt_state;

	memset(txt, 0, sizeof(*txt));
	txt->pt_state = PLIST_TXT_STATE_SCAN;

	/* return a result if the parser completed */
	if (pstate == PLIST_TXT_STATE_DONE) {
		*plistpp = ptmp;
		return 0;
	}

	/* parser didn't complete */
	plist_free(ptmp);
	return ENOENT;
}
