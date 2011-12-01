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

#define CHUNK_EXTENDSZ  (32) /* grow a chunk by 32 bytes */

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
	char *str;

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
	case PLIST_DICT:
		/* simple conversion of a string to a key */
		if (value->p_elem != PLIST_STRING) {
			plist_free(value);
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return;
		}
		str = value->p_string.ps_str;
		if (plist_dict_haskey(txt->pt_cur, str) == true) {
			plist_free(value);
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return;
		}
		value->p_elem = PLIST_KEY;
		value->p_key.pk_name = str;
		value->p_key.pk_value = NULL;

		txt->pt_cur->p_dict.pd_numkeys++;
		TAILQ_INSERT_TAIL(&txt->pt_cur->p_dict.pd_keys,
				  value, p_entry);
		value->p_parent = txt->pt_cur;
		txt->pt_cur = value;
		txt->pt_depth++;
		break;
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


int
plist_txt_parse(plist_txt_t *txt, const void *buf, size_t sz)
{
	int err;
	char *bp;
	const char *cp;
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
		while (isblank(chunk.pc_cp[0]) && chunk.pc_cp[0] != '\0' &&
		       chunk.pc_cp != chunk.pc_ep) {
			chunk.pc_cp++;
		}
		if (chunk.pc_cp == chunk.pc_ep) {
			return 0;
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
			if (txt->pt_cur == NULL) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EACCES;
			}
			if (txt->pt_cur->p_elem == PLIST_KEY) {
				_plist_txt_pop(txt);
			}
			if (txt->pt_cur->p_elem != PLIST_DICT) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EACCES;
			}
			_plist_txt_pop(txt);

			chunk.pc_cp++;
			goto nextstate;

		case ':':
			/* the value in a dictionary */
			if (txt->pt_cur == NULL ||
			    txt->pt_cur->p_elem != PLIST_KEY) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EACCES;
			}

			chunk.pc_cp++;
			goto nextstate;

		case ';':
			/* the next key in a dictionary */
			if (txt->pt_cur == NULL ||
			    txt->pt_cur->p_elem != PLIST_KEY) {
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
			chunk.pc_cp++;
			txt->pt_datacnt = 0;
			txt->pt_bufoff = 0;
			txt->pt_state = PLIST_TXT_STATE_DATA;
			goto nextstate;

		case '"':
			cp = ++chunk.pc_cp;
			/* eat until we get another '"' */
			for (;;) {
				if (cp == chunk.pc_ep) {
					/* have a string fragment */
					txt->pt_bufoff = 0;
					err = _plist_txt_buf(
						txt, cp - chunk.pc_cp);
					if (err != 0) {
						txt->pt_state =
						    PLIST_TXT_STATE_ERROR;
						return err;
					}
					txt->pt_state = PLIST_TXT_STATE_STRING;
					goto nextstate;
				}

				if (cp[0] == '\\') {
					/* escape */
					txt->pt_bufoff = 0;
					err = _plist_txt_buf(
						txt, cp - chunk.pc_cp);
					if (err != 0) {
						txt->pt_state =
						    PLIST_TXT_STATE_ERROR;
						return err;
					}
					txt->pt_state = PLIST_TXT_STATE_STRING;
					goto nextstate;
				}
				if (cp[0] == '"') {
					int cplen;

					/* have a string */
					cplen = cp - chunk.pc_cp;
					err = plist_format_new(
						&ptmp, "%.*s",
						cplen, chunk.pc_cp);
					if (err != 0) {
						txt->pt_state =
						    PLIST_TXT_STATE_ERROR;
						return err;
					}
					_plist_txt_next(txt, ptmp);
					chunk.pc_cp = &cp[1];
					goto nextstate;
				}
				cp++;
			}

			/* not reached */
			break;

		case '-':
		case '0' ... '9':
			cp = chunk.pc_cp;
			if (cp[0] == '-') {
				cp++;
			}

			/* eat all the digits */
			for (;;) {
				char *ep;
				long long ll;

				if (cp == chunk.pc_ep) {
					/* have a number fragment */
					txt->pt_bufoff = 0;
					err = _plist_txt_buf(
						txt, cp - chunk.pc_cp);
					if (err != 0) {
						txt->pt_state =
						    PLIST_TXT_STATE_ERROR;
						return err;
					}
					txt->pt_state = PLIST_TXT_STATE_NUMBER;
					goto nextstate;
				}
				if (isdigit(cp[0])) {
					cp++;
					continue;
				}
				if (cp[0] == '.' ||
				    cp[0] == 'e' || cp[0] == 'E') {
					/* have a real number */
					txt->pt_bufoff = 0;
					err = _plist_txt_buf(
						txt, cp - chunk.pc_cp);
					if (err != 0) {
						txt->pt_state =
						    PLIST_TXT_STATE_ERROR;
						return err;
					}
					txt->pt_state = PLIST_TXT_STATE_DOUBLE;
					goto nextstate;
				}

				/* try to parse this number */
				ll = strtoll(chunk.pc_cp, &ep, 0);
				if (ep != cp) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return EINVAL;
				}

				err = plist_integer_new(&ptmp, ll);
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				_plist_txt_next(txt, ptmp);
				chunk.pc_cp = cp;
				goto nextstate;
			}

			/* not reached */
			break;

		case 'T':
		case 't':
			if (chunk.pc_ep - chunk.pc_cp < strlen("rue")) {
				/* don't have enough chars */
				txt->pt_bufoff = 0;
				err = _plist_txt_buf(txt, sizeof("true"));
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				txt->pt_state = PLIST_TXT_STATE_TRUE;
				goto nextstate;
			}
			/* check the next sequence */
			if ((tolower(chunk.pc_cp[1]) != 'r') ||
			    (tolower(chunk.pc_cp[2]) != 'u') ||
			    (tolower(chunk.pc_cp[3]) != 'e')) {
				/* invalid char */
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EINVAL;
			}

			err = plist_boolean_new(&ptmp, true);
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_next(txt, ptmp);
			chunk.pc_cp += 4;
			goto nextstate;

		case 'F':
		case 'f':
			if (chunk.pc_ep - chunk.pc_cp < strlen("alse")) {
				/* don't have enough chars */
				txt->pt_bufoff = 0;
				err = _plist_txt_buf(txt, sizeof("false"));
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				txt->pt_state = PLIST_TXT_STATE_FALSE;
				goto nextstate;
			}
			/* check the next sequence */
			if ((tolower(chunk.pc_cp[1]) != 'a') ||
			    (tolower(chunk.pc_cp[2]) != 'l') ||
			    (tolower(chunk.pc_cp[3]) != 's') ||
			    (tolower(chunk.pc_cp[4]) != 'e')) {
				/* invalid char */
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EINVAL;
			}

			err = plist_boolean_new(&ptmp, false);
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_next(txt, ptmp);
			chunk.pc_cp += 5;
			goto nextstate;

		case '\0':
			if (txt->pt_top != NULL && txt->pt_depth == 0) {
				txt->pt_state = PLIST_TXT_STATE_DONE;
				return 0;
			}
			break;

		default:
			break;
		}

		/* invalid character */
		txt->pt_state = PLIST_TXT_STATE_ERROR;
		return EINVAL;

	case PLIST_TXT_STATE_DATA:
		bp = txt->pt_buf;
		for (;;) {
			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			if (txt->pt_bufoff == 0 && chunk.pc_cp[0] == '*') {
				/* switch to date parse */
				chunk.pc_cp++;
				txt->pt_state = PLIST_TXT_STATE_DATE;
				goto nextstate;
			}
			if (txt->pt_bufoff == txt->pt_bufsz) {
				err = _plist_txt_buf(txt, CHUNK_EXTENDSZ);
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				bp = txt->pt_buf;
			}
			if (isblank(chunk.pc_cp[0]) == true) {
				chunk.pc_cp++;
				continue;
			}
			if (chunk.pc_cp[0] == '>') {
				/* end of data */
				chunk.pc_cp++;
				break;
			}

#define tohex(_c)  (isdigit(_c) ? (_c) - '0' : tolower(_c)  - 'a' + 10)

			if ((txt->pt_datacnt % 2) == 0) {
				bp[txt->pt_bufoff] =
				    tohex(chunk.pc_cp[0]) << 4;
				txt->pt_datacnt++;
				chunk.pc_cp++;
				continue;
			}
			bp[txt->pt_bufoff] |= tohex(chunk.pc_cp[0]);
			txt->pt_datacnt++;
			txt->pt_bufoff++;
			chunk.pc_cp++;

#undef tohex
		}

		/* insert a data buffer */
		err = plist_data_new(&ptmp, bp,
				     txt->pt_datacnt/2 + txt->pt_datacnt%2);
		if (err != 0) {
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return err;
		}
		_plist_txt_next(txt, ptmp);

		txt->pt_state = PLIST_TXT_STATE_SCAN;
		goto nextstate;

	case PLIST_TXT_STATE_DATE:
		/* pull the date string in the buffer */
		bp = txt->pt_buf;
		for (;;) {
			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			if (txt->pt_bufoff == txt->pt_bufsz) {
				err = _plist_txt_buf(txt, CHUNK_EXTENDSZ);
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				bp = txt->pt_buf;
			}
			if (chunk.pc_cp[0] == '>') {
				bp[txt->pt_bufoff] = '\0';
				chunk.pc_cp++;
				break;
			}

			bp[txt->pt_bufoff] = chunk.pc_cp[0];
			txt->pt_bufoff++;
			chunk.pc_cp++;
		}

		/* insert a date string */
		struct tm tm;
		cp = strptime(bp, "%Y-%m-%d %H:%M:%S %z", &tm);
		if (cp == NULL) {
			/* conversion failed */
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return EINVAL;
		}
		err = plist_date_new(&ptmp, &tm);
		if (err != 0) {
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return err;
		}
		_plist_txt_next(txt, ptmp);

		txt->pt_state = PLIST_TXT_STATE_SCAN;
		goto nextstate;

		
	case PLIST_TXT_STATE_STRING:
		/* escape the string into the buffer */
		bp = txt->pt_buf;
		for (;;) {
			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			if (txt->pt_bufoff == txt->pt_bufsz) {
				err = _plist_txt_buf(txt, CHUNK_EXTENDSZ);
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				bp = txt->pt_buf;
			}
			if (txt->pt_escape) {
				switch (chunk.pc_cp[0]) {
				case '\\':
				case '/':
				case '"':
					bp[txt->pt_bufoff] = chunk.pc_cp[0];
					break;
				case 'b':
					bp[txt->pt_bufoff] = '\b';
					break;
				case 't':
					bp[txt->pt_bufoff] = '\t';
					break;
				case 'f':
					bp[txt->pt_bufoff] = '\f';
					break;
				case 'n':
					bp[txt->pt_bufoff] = '\n';
					break;
				case 'r':
					bp[txt->pt_bufoff] = '\r';
					break;
				}
				txt->pt_bufoff++;
				chunk.pc_cp++;
				txt->pt_escape = false;
				continue;
			}
			if (chunk.pc_cp[0] == '\\') {
				txt->pt_escape = true;
				chunk.pc_cp++;
				continue;
			}
			if (chunk.pc_cp[0] == '"') {
				bp[txt->pt_bufoff] = '\0';
				chunk.pc_cp++;
				break;
			}

			bp[txt->pt_bufoff] = chunk.pc_cp[0];
			txt->pt_bufoff++;
			chunk.pc_cp++;
		}

		/* insert an escaped string */
		err = plist_string_new(&ptmp, bp);
		if (err != 0) {
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return err;
		}
		_plist_txt_next(txt, ptmp);

		txt->pt_state = PLIST_TXT_STATE_SCAN;
		goto nextstate;

	case PLIST_TXT_STATE_NUMBER:
		bp = txt->pt_buf;
		for (;;) {
			char *ep;
			long long ll;

			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			if (txt->pt_bufoff == txt->pt_bufsz) {
				err = _plist_txt_buf(txt, CHUNK_EXTENDSZ);
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				bp = txt->pt_buf;
			}

			if (chunk.pc_cp[0] == '.' ||
			    chunk.pc_cp[0] == 'e' || chunk.pc_cp[0] == 'E') {
				/* switch to a real number */
				txt->pt_state = PLIST_TXT_STATE_DOUBLE;
				goto nextstate;
			}

			if (isdigit(chunk.pc_cp[0])) {
				bp[txt->pt_bufoff] = chunk.pc_cp[0];
				txt->pt_bufoff++;
				chunk.pc_cp++;
				continue;
			}
			if ((txt->pt_bufoff == 0) && (chunk.pc_cp[0] == '-')) {
				bp[txt->pt_bufoff] = chunk.pc_cp[0];
				txt->pt_bufoff++;
				chunk.pc_cp++;
				continue;
			}
			bp[txt->pt_bufoff] = 0;

			/* try to parse this number */
			ll = strtoll(bp, &ep, 0);
			if (*ep != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EINVAL;
			}

			err = plist_integer_new(&ptmp, ll);
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_next(txt, ptmp);

			txt->pt_state = PLIST_TXT_STATE_SCAN;
			goto nextstate;
		}

		/* notreached */
		break;

	case PLIST_TXT_STATE_DOUBLE:
		bp = txt->pt_buf;
		for (;;) {
			char *ep;
			double d;

			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			if (txt->pt_bufoff == txt->pt_bufsz) {
				err = _plist_txt_buf(txt, CHUNK_EXTENDSZ);
				if (err != 0) {
					txt->pt_state = PLIST_TXT_STATE_ERROR;
					return err;
				}
				bp = txt->pt_buf;
			}

			if (chunk.pc_cp[0] == '.' ||
			    chunk.pc_cp[0] == 'e' || chunk.pc_cp[0] == 'E' ||
			    chunk.pc_cp[0] == '+' || chunk.pc_cp[0] == '-' ||
			    isdigit(chunk.pc_cp[0])) {
				bp[txt->pt_bufoff] = chunk.pc_cp[0];
				txt->pt_bufoff++;
				chunk.pc_cp++;
				continue;
			}
			bp[txt->pt_bufoff] = 0;

			/* try to parse this double */
			d = strtod(bp, &ep);
			if (*ep != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return EINVAL;
			}

			err = plist_real_new(&ptmp, d);
			if (err != 0) {
				txt->pt_state = PLIST_TXT_STATE_ERROR;
				return err;
			}
			_plist_txt_next(txt, ptmp);

			txt->pt_state = PLIST_TXT_STATE_SCAN;
			goto nextstate;
		}

		/* notreached */
		break;

	case PLIST_TXT_STATE_TRUE:
		assert(strlen("true") < txt->pt_bufsz);
		assert(txt->pt_bufoff < txt->pt_bufsz);

		/* pickup the fragment in the buffer */
		bp = txt->pt_buf;
		while (txt->pt_bufoff < strlen("true")) {
			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			bp[txt->pt_bufoff] = chunk.pc_cp[0];
			txt->pt_bufoff++;
			chunk.pc_cp++;
		}
		if ((tolower(bp[0]) != 't') ||
		    (tolower(bp[1]) != 'r') ||
		    (tolower(bp[2]) != 'u') ||
		    (tolower(bp[3]) != 'e')) {
			/* invalid value */
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return EINVAL;
		}

		err = plist_boolean_new(&ptmp, true);
		if (err != 0) {
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return err;
		}
		_plist_txt_next(txt, ptmp);

		txt->pt_state = PLIST_TXT_STATE_SCAN;
		goto nextstate;

	case PLIST_TXT_STATE_FALSE:
		assert(strlen("false") < txt->pt_bufsz);
		assert(txt->pt_bufoff < txt->pt_bufsz);

		/* pickup the fragment in the buffer */
		bp = txt->pt_buf;
		while (txt->pt_bufoff < strlen("false")) {
			if (chunk.pc_cp == chunk.pc_ep) {
				return 0;
			}
			bp[txt->pt_bufoff] = chunk.pc_cp[0];
			txt->pt_bufoff++;
			chunk.pc_cp++;
		}
		if ((tolower(bp[0]) != 'f') ||
		    (tolower(bp[1]) != 'a') ||
		    (tolower(bp[2]) != 'l') ||
		    (tolower(bp[3]) != 's') ||
		    (tolower(bp[4]) != 'e')) {
			/* invalid value */
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return EINVAL;
		}

		err = plist_boolean_new(&ptmp, false);
		if (err != 0) {
			txt->pt_state = PLIST_TXT_STATE_ERROR;
			return err;
		}
		_plist_txt_next(txt, ptmp);

		txt->pt_state = PLIST_TXT_STATE_SCAN;
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
	pstate = txt->pt_state;
	ptmp = txt->pt_top;
	if (txt->pt_buf != NULL) {
		free(txt->pt_buf);
	}

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
