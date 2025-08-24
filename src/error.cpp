#include "error.h"

LexerError::LexerError(const std::string &msg, size_t loc)
    : msg(msg), loc(loc) {
  if (loc != 0) {
    formatted = msg + ", At Line: " + std::to_string(loc);
  } else {
    formatted = msg;
  }
}

const char *LexerError::what() const noexcept { return formatted.c_str(); }
