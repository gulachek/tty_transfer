/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */
#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <regex>
#include <string>
#include <strstream>

#include <os/log.h>
#include <util.h>
#include <uuid/uuid.h>

#include "tty_transfer.h"

// https://www.rfc-editor.org/rfc/rfc9562.html
#define UUID_KEY "68338148-030e-436c-89eb-9f905860f83b"
#define UUID_KEY_UPPER "68338148-030E-436C-89EB-9F905860F83B"
#define UUID_KEY2 "11111111-2222-3333-4444-555555555555"
#define UUID_VAL "f81d4fae-7dec-11d0-a765-00a0c91e6bf6"

#define UUID_RE "[[:xdigit:]]{8}-([[:xdigit:]]{4}-){3}[[:xdigit:]]{12}"

std::string read_until_csi6n(int fd);
void send_token(int fd, const std::string &token_key, const char *token_val);

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
                      "\e[2;1h" // not R
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

TEST(TtyTransferParser, CompletesParsingWithoutToken) {
  const char *input = "\e[2;1R";

  tty_transfer_parser *p = tty_transfer_parser_alloc();

  int nused = tty_transfer_parser_feed(p, input, std::strlen(input));

  EXPECT_EQ(nused, std::strlen(input));

  EXPECT_FALSE(tty_transfer_parser_token_for_key(p, UUID_KEY));

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

TEST(TtyTransferRequestIoToken, ParsesTokenWhenAvailable) {
  uuid_t tokenval;
  uuid_generate_random(tokenval);
  char host_token[37];
  uuid_unparse(tokenval, host_token);

  int pty_master;
  pid_t pid = forkpty(&pty_master, nullptr, nullptr, nullptr);
  EXPECT_NE(pid, -1);

  if (pid == 0) {
    char token[37];
    auto ret = tty_transfer_request_io_token(token, sizeof(token));
    if (ret != TTY_TRANSFER_OK)
      std::exit(ret);

    os_log(OS_LOG_DEFAULT, "TESTING: token '%{public}s'", token);
    os_log(OS_LOG_DEFAULT, "TESTING: host_token '%{public}s'", host_token);
    if (std::strcmp(token, host_token) != 0)
      std::exit(1);

    std::exit(0);
  } else {
    auto out = read_until_csi6n(pty_master);
    EXPECT_GT(out.length(), 0) << "Child process did not write to stdout";

    std::regex re("RequestTransferIOToken=(" UUID_RE ")");
    std::cmatch m;
    EXPECT_TRUE(std::regex_search(out.c_str(), m, re)) << "Did not match RE";

    if (!m.empty()) {
      auto token_key = m[1].str();
      send_token(pty_master, token_key, host_token);
    }

    int exit_info;
    pid_t exited_pid = ::wait(&exit_info);
    EXPECT_EQ(pid, exited_pid);
    EXPECT_EQ(WEXITSTATUS(exit_info), 0)
        << "Child process exited with nonzero status";
    ::close(pty_master);
  }
}

std::string read_until_csi6n(int fd) {
  std::string csi6n = "\e[6n";
  char buf[256];
  size_t bufsz = sizeof(buf) / sizeof(char);

  int total = 0;
  while (total < bufsz) {
    int nread = ::read(fd, &buf[total], bufsz - total);
    if (nread < 1)
      return "";

    total += nread;
    if (total >= csi6n.length()) {
      std::string haystack{buf, buf + total};
      auto pos = haystack.rfind(csi6n);
      if (pos != std::string::npos)
        return haystack;
    }
  }

  return "";
}

void send_token(int fd, const std::string &token_key, const char *token_val) {
  std::ostringstream os;
  os << "\e]1337;IOToken=" << token_key << ';' << token_val
     << "\e\\"
        "\e[1;2R";
  auto data = os.str();
  os_log(OS_LOG_DEFAULT, "TESTING: sending key=%{public}s val=%{public}s",
         token_key.c_str(), token_val);
  ::write(fd, data.data(), data.size());
}
