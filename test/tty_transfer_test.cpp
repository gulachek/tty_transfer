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
#define UUID_KEY_UPPER "68338148-030E-436C-89EB-9F905860F83B"
#define UUID_KEY2 "11111111-2222-3333-4444-555555555555"
#define UUID_VAL "f81d4fae-7dec-11d0-a765-00a0c91e6bf6"

TEST(TtyTransferParser, ParsesToken) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "bar"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, ParsesTokenWithSpacesInBetween) {
  const char *input =
      "foo"
      "\e] 1337\t\t;\n\nIOToken  =\v\t" UUID_KEY "  ;\n" UUID_VAL "    \e\\"
      "bar"
      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, MatchesIOTokenCaseInsensitively) {
  const char *input = "foo"
                      "\e]1337;iotOKEN=" UUID_KEY ";" UUID_VAL "\e\\"
                      "bar"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, MatchesUUIDCaseInsensitively) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY_UPPER ";" UUID_VAL "\e\\"
                      "bar"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, CanParseIncrementally) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "bar"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  auto n = std::strlen(input);
  for (int i = 0; i < n - 1; ++i) {
    int nused = tty_transfer_parser_feed(p, &input[i], sizeof(char));
    EXPECT_EQ(nused, 0);
  }

  int nused = tty_transfer_parser_feed(p, &input[n - 1], 1);

  EXPECT_EQ(nused, 1);

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, LastInputTokenWins) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e]1337;IOToken=" UUID_KEY2 ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY2);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, ExtraOSCBetweenTokenAndCSIInvalidatesToken) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e]1337;AnothaOne\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, HasNoTokenAfterReset) {
  const char *input = "foo"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();
  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));
  EXPECT_EQ(nused, std::strlen(input));

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
  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));
  EXPECT_EQ(nused, std::strlen(input));

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, HasNoTokenWithoutExpectedPrefix) {
  const char *input = "foo"
                      "\e]abcd;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();
  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));
  EXPECT_EQ(nused, std::strlen(input));

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, FailsToFindTokenWithTruncatedUUIDKeyForLookup) {
  const char *input = "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  char key_trunc[] = UUID_KEY;
  key_trunc[10] = '\0';

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, key_trunc));

  tty_transfer_parser_free(p);
}

TEST(TtyTransferParser, IgnoresExtraCSI) {
  const char *input = "\e[999;888k"
                      "\e]1337;IOToken=" UUID_KEY ";" UUID_VAL "\e\\"
                      "\e[123;145h"
                      "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  std::string tok = tty_transfer_parser_token_for_key(p, UUID_KEY);

  EXPECT_EQ(tok, UUID_VAL);

  tty_transfer_parser_free(p);
}
