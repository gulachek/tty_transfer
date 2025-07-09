/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#if defined(__linux__)
// enable cfmakeraw
#define _DEFAULT_SOURCE
#endif

#include "tty_transfer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

enum tty_sequence_type { normal, csi, osc };

struct tty_transfer_parser_ {
  int is_esc;
  enum tty_sequence_type seq_type;
  char osc_str[128]; // 1337;IOToken=<uuid-key>;<uuid-val>
  char *osc_str_back;
  const char *key;
  const char *val;
};

tty_transfer_parser *tty_transfer_parser_alloc() {
  tty_transfer_parser *p = malloc(sizeof(tty_transfer_parser));
  tty_transfer_parser_reset(p);
  return p;
}

void tty_transfer_parser_free(tty_transfer_parser *p) { free(p); }

void tty_transfer_parser_reset(tty_transfer_parser *p) {
  p->osc_str[0] = '\0';
  p->osc_str_back = &p->osc_str[0];
  p->is_esc = 0;
  p->seq_type = normal;
  p->key = NULL;
  p->val = NULL;
}

static const char *parse_literal_nocase(const char *start, const char *end,
                                        const char *lower_literal) {
  const char *it = start;
  while (it < end && isspace(*it)) {
    ++it;
  }

  int len = strlen(lower_literal);
  for (int i = 0; i < len; ++i) {
    if (it >= end)
      return NULL;
    if (tolower(*it) != lower_literal[i])
      return NULL;
    ++it;
  }

  return it;
}

static const char *parse_literal(const char *start, const char *end,
                                 const char *literal) {
  const char *it = start;
  while (it < end && isspace(*it)) {
    ++it;
  }

  int len = strlen(literal);
  for (int i = 0; i < len; ++i) {
    if (it >= end)
      return NULL;
    if (*it != literal[i])
      return NULL;
    ++it;
  }

  return it;
}

static const char *parse_xdigits(const char *start, const char *end,
                                 int ndigits) {
  const char *it = start;
  for (int i = 0; i < ndigits; ++i) {
    if (it >= end)
      return NULL;
    if (!isxdigit(*it))
      return NULL;
    ++it;
  }

  return it;
}

static const char *parse_char(const char *start, const char *end, char c) {
  if (start >= end)
    return NULL;
  if (*start != c)
    return NULL;
  return start + 1;
}

static const char *parse_uuid(const char *start, const char *end,
                              const char **uuid_start) {
  const char *it = start;
  while (it < end && isspace(*it)) {
    ++it;
  }

  if (uuid_start)
    *uuid_start = it;

  if (!(it = parse_xdigits(it, end, 8)))
    return NULL;
  if (!(it = parse_char(it, end, '-')))
    return NULL;
  if (!(it = parse_xdigits(it, end, 4)))
    return NULL;
  if (!(it = parse_char(it, end, '-')))
    return NULL;
  if (!(it = parse_xdigits(it, end, 4)))
    return NULL;
  if (!(it = parse_char(it, end, '-')))
    return NULL;
  if (!(it = parse_xdigits(it, end, 4)))
    return NULL;
  if (!(it = parse_char(it, end, '-')))
    return NULL;
  if (!(it = parse_xdigits(it, end, 12)))
    return NULL;

  return it;
}

static const char *parse_end_ws(const char *start, const char *end) {
  const char *it = start;
  while (it < end && isspace(*it))
    ++it;

  if (it != end)
    return NULL;
  return it;
}

static void tty_transfer_parser_parse_io_token(tty_transfer_parser *p) {
  p->key = NULL;
  p->val = NULL;

  const char *key = NULL;
  const char *val = NULL;

  const char *it = p->osc_str;
  const char *end = p->osc_str_back;
  if (!(it = parse_literal(it, end, "1337")))
    return;

  if (!(it = parse_literal(it, end, ";")))
    return;

  if (!(it = parse_literal_nocase(it, end, "iotoken")))
    return;

  if (!(it = parse_literal(it, end, "=")))
    return;

  if (!(it = parse_uuid(it, end, &key)))
    return;

  if (!(it = parse_literal(it, end, ";")))
    return;

  if (!(it = parse_uuid(it, end, &val)))
    return;

  if (!(it = parse_end_ws(it, end)))
    return;

  p->key = key;
  p->val = val;

  int offset = p->val - p->osc_str;
  p->osc_str[offset + 36] = '\0'; // terminate val UUID str
}

static void tty_transfer_parser_push_strchr(tty_transfer_parser *p, char c) {
  if (p->osc_str_back - &p->osc_str[0] + 1 >=
      (sizeof(p->osc_str) / sizeof(char))) {
    return;
  }

  *p->osc_str_back = c;
  ++p->osc_str_back;
  *p->osc_str_back = '\0';
}

static int tty_transfer_parser_feed_char(tty_transfer_parser *p, char c) {
  if (p->seq_type == osc) {
    if (p->is_esc) {
      // ST
      if (c == '\\') {
        tty_transfer_parser_parse_io_token(p);
        p->seq_type = normal;
      } else {
        tty_transfer_parser_push_strchr(p, '\e');
        tty_transfer_parser_push_strchr(p, c);
      }
      p->is_esc = 0;
    } else {
      if (c == '\e') {
        p->is_esc = 1;
      } else {
        tty_transfer_parser_push_strchr(p, c);
      }
    }
  } else if (p->seq_type == csi) {
    // See ECMA 48 Section 5.4 d). Final byte is 04/00 to 07/14
    if (c >= 0x40 && c <= 0x7e) {
      p->seq_type = normal;
      return c == 'R'; // end of cursor position report
    }
  } else {
    if (p->is_esc) {
      if (c == ']') {
        p->seq_type = osc;
        p->osc_str_back = &p->osc_str[0];
        p->osc_str[0] = '\0';
      } else if (c == '[') {
        p->seq_type = csi;
      }

      p->is_esc = 0;
    } else {
      if (c == '\e') {
        p->is_esc = 1;
      }
    }
  }

  return 0;
}

int tty_transfer_parser_feed(tty_transfer_parser *p, const void *bytes,
                             size_t nbytes) {
  for (int i = 0; i < nbytes; ++i) {
    char c = ((const char *)bytes)[i];
    if (tty_transfer_parser_feed_char(p, c))
      return i + 1;
  }

  return 0;
}

const char *tty_transfer_parser_token_for_key(const tty_transfer_parser *p,
                                              const char *key) {
  if (!(p->key && p->val))
    return NULL;

  // expect 36 char UUID key
  if (strlen(key) != 36)
    return NULL;

  if (p->osc_str_back - p->key < 36)
    return NULL;

  for (int i = 0; i < 36; ++i) {
    if (tolower(key[i]) != tolower(p->key[i]))
      return NULL;
  }

  return p->val;
}

#if defined(__APPLE__) || defined(__linux__)
#include "tty_transfer/private/impl/tty_transfer_posix.c"
#else
#error "Platform not supported!"
#endif
