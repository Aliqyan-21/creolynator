#include "b_lexer.h"
#include "i_lexer.h"
#include "iostream"
#include "utils.h"

int main(int argc, char *argv[]) {
  Args args = parse_args(argc, argv);
  try {
    B_Lexer b_lexer(args.filename);
    b_lexer.b_tokenize();
    b_lexer.print_tokens();
    I_Lexer i_lexer(b_lexer.get_tokens());
    i_lexer.i_tokenize();
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
