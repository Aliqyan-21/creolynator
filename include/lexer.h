#ifndef LEXER_H
#define LEXER_H

#include <optional>
#include <stdint.h>
#include <string>
#include <vector>

enum class BlockTokenType {
  HEADING,
  NEWLINE,
  ENDOF,
};

struct Token {
  BlockTokenType type;
  size_t loc;
  std::optional<std::string> text; // raw text
  std::optional<int> level;        // for heading
};

class Lexer {
public:
  explicit Lexer(const std::string &filepath);
  void tokenize();
  void print_tokens();
  std::vector<Token> get_tokens();

private:
  std::vector<Token> tokens;
  std::string creole_data;
  size_t pos;
  size_t loc;

  /*=== Printing Functions ===*/
  std::string token_to_string(BlockTokenType type);

  /*=== Reading Functions ===*/
  void read_heading();

  /*=== Helper Functions ===*/
  inline bool end();
  void advance();
  char peek();
  inline bool is_newline();
  inline bool is_whites();
};

#endif // !LEXER_H
