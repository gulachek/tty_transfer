/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */
#ifndef TTY_TRANSFER_H
#define TTY_TRANSFER_H

#include <stddef.h>

#ifndef TTY_TRANSFER_API
#define TTY_TRANSFER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Test function
 * @return 4
 */
TTY_TRANSFER_API int tty_transfer_foo(void);

enum tty_sequence_type { normal, csi, osc };

typedef struct {
  int is_esc;
  enum tty_sequence_type seq_type;
  char token[42]; // 32 hex chars + 4 '-' + 1 null terminator + "1337;"
  char *token_back;
} tty_transfer_parser;

TTY_TRANSFER_API void tty_transfer_parser_reset(tty_transfer_parser *p);

TTY_TRANSFER_API int tty_transfer_parser_feed(tty_transfer_parser *p,
                                              const void *bytes, size_t nbytes);

TTY_TRANSFER_API const char *
tty_transfer_parser_token(const tty_transfer_parser *p);

#ifdef __cplusplus
}
#endif

#endif
