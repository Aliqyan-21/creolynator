#include "lexer.h"
#include "utils.h"

Lexer::Lexer(const std::string &filepath) : pos(0), loc(1) {
  data = read_creole_file(filepath);
}
