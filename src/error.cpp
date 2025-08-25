#include "error.h"

B_LexerError::B_LexerError(const std::string &msg, size_t loc)
    : msg(msg), loc(loc) {
  if (loc != 0) {
    formatted = "Block Lexer: " + msg + ", At Line: " + std::to_string(loc);
  } else {
    formatted = msg;
  }
}

const char *B_LexerError::what() const noexcept { return formatted.c_str(); }

I_LexerError::I_LexerError(const std::string &msg, size_t loc)
    : msg(msg), loc(loc) {
  if (loc != 0) {
    formatted = "Inline Lexer: " + msg + ", At Line: " + std::to_string(loc);
  } else {
    formatted = msg;
  }
}

const char *I_LexerError::what() const noexcept { return formatted.c_str(); }
