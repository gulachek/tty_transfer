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

typedef struct tty_transfer_parser_ tty_transfer_parser;

TTY_TRANSFER_API tty_transfer_parser *tty_transfer_parser_alloc();
TTY_TRANSFER_API void tty_transfer_parser_free(tty_transfer_parser *p);

TTY_TRANSFER_API void tty_transfer_parser_reset(tty_transfer_parser *p);

TTY_TRANSFER_API int tty_transfer_parser_feed(tty_transfer_parser *p,
                                              const void *bytes, size_t nbytes);

TTY_TRANSFER_API const char *
tty_transfer_parser_token(const tty_transfer_parser *p);

#ifdef __cplusplus
}
#endif

#endif
