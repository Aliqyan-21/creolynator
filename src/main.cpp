#include "b_lexer.h"
#include "error.h"
#include "iostream"
#include "migr_semantic.h"
#include "migr_structural.h"
#include "utils.h"
#include <fstream>

int main(int argc, char *argv[]) {
  Args args = parse_args(argc, argv);
  try {
    BLexer blexer(args.filename);
    blexer.b_tokenize();
    StructuralLayer ll;
    ll.build_from_tokens(blexer.get_tokens());
    SemanticLayer sm;
    sm.extract_semantics(ll);

    // Use the built-in debug function
    sm.print_semantic_info(true);

    std::ofstream sm_out("serialize.txt");
    sm.serialize(sm_out);
  } catch (const CNError &e) {
    std::cout << e.format() << std::endl;
  }
  return 0;
}
