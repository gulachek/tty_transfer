/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */
#include <cstring>
#include <gtest/gtest.h>
#include <string>

#include "tty_transfer.h"

// https://www.rfc-editor.org/rfc/rfc9562.html
#define UUID_KEY "68338148-030e-436c-89eb-9f905860f83b"
#define UUID_VAL "f81d4fae-7dec-11d0-a765-00a0c91e6bf6"

TEST(TtyTransferParser, ParsesToken) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int done = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_TRUE(done);

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, HasNoTokenAfterReset) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();
  int done = tty_transfer_parser_feed(p, input, std::strlen(input));
  EXPECT_TRUE(done);

  tty_transfer_parser_reset(p);

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, ExtraBytesAtEndOfKeyMakesInvalid) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "abcdefg"
                      "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();
  int done = tty_transfer_parser_feed(p, input, std::strlen(input));
  EXPECT_TRUE(done);

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, HasNoTokenWithoutExpectedPrefix) {
  const char *input = "foo"
                      "\e]abcd;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();
  int done = tty_transfer_parser_feed(p, input, std::strlen(input));
  EXPECT_TRUE(done);

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  tty_transfer_parser_free(p);
}
