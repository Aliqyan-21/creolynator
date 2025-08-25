#ifndef I_LEXER_H
#define I_LEXER_H

#include <string>

enum class InlineTokenType {
  Normal,
};

struct I_Token {
  InlineTokenType type;
  std::string text;
};

class I_Lexer {
public:
  I_Lexer(const std::string &raw_text);
private:
  std::string raw_text;
  size_t pos;
};

#endif //! I_LEXER_H
