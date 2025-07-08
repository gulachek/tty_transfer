/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */
#include "tty_transfer.h"
#include <string.h>

TTY_TRANSFER_API void tty_transfer_parser_reset(tty_transfer_parser *p) {
  p->token[0] = '\0';
  p->token_back = &p->token[0];
  p->is_esc = 0;
  p->seq_type = normal;
}

static void tty_transfer_parser_parse_osc_uuid(tty_transfer_parser *p) {}

static void tty_transfer_parser_push_strchr(tty_transfer_parser *p, char c) {
  if (p->token_back - &p->token[0] + 1 >= (sizeof(p->token) / sizeof(char))) {
    return;
  }

  *p->token_back = c;
  ++p->token_back;
  *p->token_back = '\0';
}

static int tty_transfer_parser_feed_char(tty_transfer_parser *p, char c) {
  if (p->seq_type == osc) {
    if (p->is_esc) {
      // ST
      if (c == '\\') {
        tty_transfer_parser_parse_osc_uuid(p);
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

// TODO if feeding multiple bytes, do we need to indicate where parsing ended?
int tty_transfer_parser_feed(tty_transfer_parser *p, const void *bytes,
                             size_t nbytes) {
  for (int i = 0; i < nbytes; ++i) {
    char c = ((const char *)bytes)[i];
    if (tty_transfer_parser_feed_char(p, c))
      return 1;
  }

  return 0;
}

const char *tty_transfer_parser_token(const tty_transfer_parser *p) {
  if (strncmp("1337;", p->token, 5) == 0) {
    return &p->token[5];
  }

  return NULL;
}
