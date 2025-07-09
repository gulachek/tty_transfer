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
 * Type that encapsulates parsing tty I/O transfer tokens
 */
typedef struct tty_transfer_parser_ tty_transfer_parser;

/**
 * Allocate a tty_transfer_parser
 * @returns The newly allocated structure or NULL
 * @remarks This implicitly initializes the parser with
 * tty_transfer_parser_reset
 */
TTY_TRANSFER_API tty_transfer_parser *tty_transfer_parser_alloc();

/**
 * Free a tty_transfer_parser
 */
TTY_TRANSFER_API void tty_transfer_parser_free(tty_transfer_parser *p);

/**
 * Reset a tty_transfer_parser state as if newly initialized
 * @param[in] p The parser
 */
TTY_TRANSFER_API void tty_transfer_parser_reset(tty_transfer_parser *p);

/**
 * Feed characters to a parser to incrementally parse an I/O token
 * @param[in] p The parser
 * @param[in] bytes The character bytes to parse
 * @param[in] nbytes The number of character bytes to parse
 * @returns Byte offset in bytes past the end of the sequence if done parsing
 * the I/O token, 0 if more data is needed
 */
TTY_TRANSFER_API int tty_transfer_parser_feed(tty_transfer_parser *p,
                                              const void *bytes, size_t nbytes);

/**
 * Access a parsed I/O token
 * @param[in] p The parser
 * @param[in] key The UUID key associated with the IO token request (like
 * 68338148-030e-436c-89eb-9f905860f83b)
 * @returns A pointer to the parsed token, or NULL
 * @remarks The returned pointer is invalidated by calling
 * tty_transfer_parser_free
 */
TTY_TRANSFER_API const char *
tty_transfer_parser_token_for_key(const tty_transfer_parser *p,
                                  const char *key);

/**
 * Constants representing error conditions
 */
typedef enum tty_transfer_errno {
  /** Success. No error */
  TTY_TRANSFER_OK = 0,
  /** stdin is not a tty */
  TTY_TRANSFER_STDIN_NOT_TTY = 1,
  /** stdout is not a tty */
  TTY_TRANSFER_STDOUT_NOT_TTY = 2,
  /** stdin and stdout represent different devices */
  TTY_TRANSFER_STDIO_MISMATCH = 3,
  /** bad write */
  TTY_TRANSFER_BAD_WRITE = 4,
  /** bad read */
  TTY_TRANSFER_BAD_READ = 5,
  /** bad memory allocation */
  TTY_TRANSFER_BAD_ALLOC = 6,
  /** no I/O token received from host tty */
  TTY_TRANSFER_NO_TOKEN = 6,
  /** token was truncated */
  TTY_TRANSFER_TOKEN_TRUNCATED = 7,
  /** timed out */
  TTY_TRANSFER_TIMEOUT = 8,
} tty_transfer_errno;

/**
 * Synchronously request an IO token to transfer the TTY
 * @param[out] token_buf The buffer to hold the null terminated output token
 * @param[in] token_buf_size The size of token_buf in chars. This must be at
 * least 37 to hold a null terminated formatted UUID
 * @returns An error code constant
 */
TTY_TRANSFER_API tty_transfer_errno
tty_transfer_request_io_token(char *token_buf, size_t token_buf_size);

#ifdef __cplusplus
}
#endif

#endif
