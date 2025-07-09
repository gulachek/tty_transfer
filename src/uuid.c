/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "tty_transfer/private/uuid.h"

#if defined(__linux__)
#include <uuid/uuid.h>
#elif defined(__APPLE__)
#include <uuid/uuid.h>
#else
#error "Platform not supported!"
#endif

#if defined(__linux__) || defined(__APPLE__)
int tty_transfer_uuid_generate(char *buf, size_t bufsz) {
  if (bufsz < TTY_TRANSFER_UUID_SIZE)
    return 0;

  uuid_t uuid;
  uuid_generate_random(uuid);
  uuid_unparse(uuid, buf);
  return 1;
}
#endif
