#include "i_lexer.h"
#include "error.h"
#include <iostream>

I_Lexer::I_Lexer(const std::string &raw_text, size_t loc)
    : raw_text(raw_text), pos(0), loc(loc) {}

void I_Lexer::i_tokenize() {
  while (!end()) {
    std::cout << peek() << ", ";
    advance();
  }
  std::cout << std::endl;
}

/*=== Helper Functions ===*/
bool I_Lexer::end() { return pos >= raw_text.size(); }

void I_Lexer::advance() {
  if (!end()) {
    pos++;
  } else {
    throw I_LexerError("Unexpected end of tokens while advancing", loc);
  }
}

char I_Lexer::peek() {
  if (!end()) {
    return raw_text[pos];
  } else {
    throw I_LexerError("Unexpected end of tokens while taking a peek", loc);
  }
}

char I_Lexer::lookahead() {
  if (pos + 1 < raw_text.size()) {
    return raw_text[pos];
  } else {
    throw I_LexerError("Unexpected end of tokens while looking ahead", loc);
  }
}
