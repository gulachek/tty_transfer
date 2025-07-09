/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

// Assume posix!!!
#include "tty_transfer.h"

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <os/log.h>

tty_transfer_errno tty_transfer_request_io_token(char *token_buf,
                                                 size_t token_buf_size) {
  if (!isatty(STDIN_FILENO)) {
    os_log_debug(OS_LOG_DEFAULT, "TESTING: stdin not tty");
    return TTY_TRANSFER_STDIN_NOT_TTY;
  }

  // Make raw terminal
  // TODO - reset this
  struct termios tattr_orig, tattr;
  tcgetattr(STDIN_FILENO, &tattr_orig);
  tattr = tattr_orig;
  cfmakeraw(&tattr);
  tcsetattr(STDIN_FILENO, TCSADRAIN, &tattr);

  uuid_t token_key;
  uuid_generate_random(token_key);
  char token_key_buf[37];
  uuid_unparse(token_key, token_key_buf);

  char buf[256];
  size_t bufsz = sizeof(buf) / sizeof(char);
  buf[0] = '\0';

  strlcat(buf, "\e]1337;RequestTransferIOToken=", bufsz);
  strlcat(buf, token_key_buf, bufsz);
  strlcat(buf,
          "\e\\"
          "\e[6n",
          bufsz);

  if (write(STDOUT_FILENO, buf, strlen(buf)) == -1)
    return TTY_TRANSFER_BAD_WRITE;

  tty_transfer_parser *p = tty_transfer_parser_alloc();
  if (!p)
    return TTY_TRANSFER_BAD_ALLOC;

  tty_transfer_errno out = TTY_TRANSFER_OK;

  struct timespec start;

  // TODO check retval
  clock_gettime(CLOCK_MONOTONIC, &start);

  while (1) {
    os_log_debug(OS_LOG_DEFAULT, "TESTING: loop");
    struct timespec now;

    // TODO check retval
    clock_gettime(CLOCK_MONOTONIC, &now);

    size_t ms_diff = 1000 * (now.tv_sec - start.tv_sec) +
                     (now.tv_nsec - start.tv_nsec) / 1000000;

    if (ms_diff >= 500) {
      out = TTY_TRANSFER_TIMEOUT;
      os_log_debug(OS_LOG_DEFAULT, "TESTING: timeout");
      break;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = (500 - ms_diff) * 1000;

    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1) {
      // TODO log this
      out = TTY_TRANSFER_BAD_READ;
      break;
    }

    if (!FD_ISSET(STDIN_FILENO, &readfds)) {
      continue;
    }

    ssize_t nread = read(STDIN_FILENO, buf, bufsz);
    if (nread < 1) {
      out = TTY_TRANSFER_BAD_READ;
      break;
    }

    // done parsing
    if (tty_transfer_parser_feed(p, buf, nread)) {
      const char *token = tty_transfer_parser_token_for_key(p, token_key_buf);
      os_log_debug(OS_LOG_DEFAULT, "TESTING: token %{public}s", token);

      if (token) {
        int ncpy = strlcpy(token_buf, token, token_buf_size);
        if (ncpy >= token_buf_size) {
          out = TTY_TRANSFER_TOKEN_TRUNCATED;
        }
      } else {
        out = TTY_TRANSFER_NO_TOKEN;
      }

      break;
    }
  }

  tcsetattr(STDIN_FILENO, TCSADRAIN, &tattr_orig);
  tty_transfer_parser_free(p);
  return out;
}
