#ifndef I_LEXER_H
#define I_LEXER_H

#include <string>
#include <vector>

enum class InlineTokenType {
  Normal,
};

struct I_Token {
  InlineTokenType type;
  std::string text;
};

class I_Lexer {
public:
  I_Lexer(const std::string &raw_text, size_t loc);
  void i_tokenize();
private:
  std::vector<InlineTokenType> inlnes; // our end product
  std::string raw_text;
  size_t pos;
  size_t loc;

  /*=== Helper Functions ===*/
  bool end();
  void advance();
  char peek();
  char lookahead();
};

#endif //! I_LEXER_H
