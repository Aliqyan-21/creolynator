#ifndef I_LEXER_H
#define I_LEXER_H

#include "b_lexer.h"
#include <string>

enum class INLINETOKENTYPE {
  ITALICS,
};

struct I_Token {
  INLINETOKENTYPE type;
  std::string text; // tokenized text
};

class I_Lexer {
public:
  I_Lexer(const std::vector<B_Token> &b_tokens);
  void i_tokenize();

private:
  std::vector<I_Token> i_tokens; // inline tokens
  std::vector<B_Token> b_tokens; // block tokens from B_Lexer
  size_t pos;                    // pos of token in b_tokens
  size_t c_pos;                  // pos of char in text of token

  /*=== Tokenizing Functions ===*/
  void tokenize_text();

  /*=== Reading Functions ===*/
  void read_italics();

  /*=== Helper Functions ===*/
  inline bool end();     // is end of b_tokens?
  bool text_end();       // is end of b_tokens.text?
  void advance();        // advance pos
  void text_advance();   // advance c_pos
  B_Token peek();        // peek token of b_token
  char text_peek();      // peek char of b_token.text
  char text_lookahead(); // look ahead of c_pos of text
  inline size_t get_loc();
};

#endif // !I_LEXER_H
