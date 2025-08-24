#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <string>

enum class BlockTokenType {
  HEADING,
};

struct Token {
  BlockTokenType type;
  std::string text; // raw text
  uint8_t level; // for heading
  size_t loc;
};

class Lexer {
public:
  Lexer(const std::string &filepath);
  void tokenize();

private:
  std::string data;
  size_t pos;
  size_t loc;
};

#endif // !LEXER_H
