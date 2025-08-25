#include "i_lexer.h"
#include "error.h"

I_Lexer::I_Lexer(const std::vector<B_Token> &b_tokens)
    : b_tokens(b_tokens), pos(0) {}

/*=== Tokenizing Functions ===*/
void I_Lexer::i_tokenize() {
  while (!end()) {
    if (peek().text.has_value()) {
      tokenize_text();
      advance();
    } else {
      advance();
    }
  }
}

void I_Lexer::tokenize_text() {
  while (!text_end()) {
    if (text_peek() == '/' && text_lookahead() == '/') {
      read_italics();
    }
    text_advance();
  }
}

/*=== Reading Functions ===*/
void I_Lexer::read_italics() {}

/*=== Helper Functions ===*/
inline bool I_Lexer::end() { return pos >= b_tokens.size(); }

bool I_Lexer::text_end() {
  if (c_pos >= peek().text.value().size()) {
    c_pos = 0;
    return true;
  }
  return false;
}

inline void I_Lexer::advance() {
  if (!end()) {
    pos++;
  } else {
    throw I_LexerError("Unexpected end of tokens while advancing", get_loc());
  }
}

void I_Lexer::text_advance() {
  if (!text_end()) {
    c_pos++;
  } else {
    throw I_LexerError("Unexpected end of char while advancing", get_loc());
  }
}

inline size_t I_Lexer::get_loc() {
  if (!end()) {
    return b_tokens[pos].loc;
  }
  throw I_LexerError("Unexpected end of tokens while getting loc", 0);
}

B_Token I_Lexer::peek() {
  if (!end()) {
    return b_tokens[pos];
  }
  throw B_LexerError("Unexpected end of tokens while taking a peek", 0);
}

char I_Lexer::text_peek() {
  if (!text_end()) {
    return peek().text.value()[c_pos];
  }
  throw B_LexerError("Unexpected end of char while taking a peek", get_loc());
}

char I_Lexer::text_lookahead() {
  if (c_pos + 1 < peek().text.value().size()) {
    return peek().text.value()[c_pos + 1];
  }
  throw B_LexerError("Unexpected end of char while looking ahead in text",
                     get_loc());
}

inline B_Token peek();
