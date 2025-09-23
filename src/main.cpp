#include "b_lexer.h"
#include "error.h"
#include "iostream"
#include "utils.h"

int main(int argc, char *argv[]) {
  Args args = parse_args(argc, argv);
  try {
    BLexer blexer(args.filename);
    blexer.b_tokenize();
    blexer.process_inline_tokens();
    blexer.print_tokens();
  } catch (const CNError &e) {
    std::cout << e.format() << std::endl;
  }
  return 0;
}
