#include <stdio.h>
#include <tty_transfer.h>

int main() {
  tty_transfer_parser *p = tty_transfer_parser_alloc();
  tty_transfer_parser_reset(p);
  tty_transfer_parser_free(p);
  return 0;
}
