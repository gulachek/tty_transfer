/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#ifndef TTY_TRANSFER_PRIVATE_UUID_H
#define TTY_TRANSFER_PRIVATE_UUID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/** Size for allocating memory for a formatted UUID string */
#define TTY_TRANSFER_UUID_SIZE 37

/**
 * Generate a random UUID and format in buf
 * @param[out] buf The buffer to store the formatted UUID
 * @param[in] bufsz The size of the buffer argument. Must be at least
 * TTY_TRANSFER_UUID_SIZE
 * @returns 1 on success, 0 on failure
 */
int tty_transfer_uuid_generate(char *buf, size_t bufsz);

#ifdef __cplusplus
}
#endif

#endif
