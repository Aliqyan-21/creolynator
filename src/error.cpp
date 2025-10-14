#include "error.h"
#include <sstream>

CNError::CNError(const std::string &msg, size_t loc) : msg(msg), loc(loc) {}

std::string CNError::format() const {
  std::stringstream ss;
  ss << "Error at " << loc << " : " << what();
  return ss.str();
}

B_LexerError::B_LexerError(const std::string &msg, size_t loc)
    : CNError("Block Lexer: " + msg, loc) {}

I_LexerError::I_LexerError(const std::string &msg, size_t loc)
    : CNError("Inline Lexer: " + msg, loc) {}

MIGRError::MIGRError(const std::string &msg, size_t loc)
    : CNError("[MIGR ERROR] " + msg, loc) {}
