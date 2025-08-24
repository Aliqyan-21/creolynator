#include "iostream"
#include "lexer.h"
#include "utils.h"

int main(int argc, char *argv[]) {
  Args args = parse_args(argc, argv);
  try {
    Lexer lexer(args.filename);
    lexer.tokenize();
    lexer.print_tokens();
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
