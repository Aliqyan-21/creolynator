#include "b_lexer.h"
#include "error.h"
#include "iostream"
#include "migr_structural.h"
#include "utils.h"
#include <fstream>

int main(int argc, char *argv[]) {
  Args args = parse_args(argc, argv);
  try {
    BLexer blexer(args.filename);
    blexer.b_tokenize();
    // blexer.process_inline_tokens();
    blexer.print_tokens();
    StructuralLayer ll;
    ll.build_from_tokens(blexer.get_tokens());
    std::ofstream outf("serialize.txt");
    ll.serialize(outf);
  } catch (const CNError &e) {
    std::cout << e.format() << std::endl;
  }
  return 0;
}
