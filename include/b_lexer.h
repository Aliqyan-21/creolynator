#ifndef B_LEXER_H
#define B_LEXER_H

#include <optional>
#include <string>
#include <vector>

enum class BlockTokenType {
  HEADING,
  NEWLINE,
  ULISTITEM, // unordered
  OLISTITEM, // ordered
  HORIZONTALRULE,
  PARAGRAPH,
  VERBATIMBLOCK, // just like a code block
  ENDOF,
};

struct BToken {
  BlockTokenType type;
  size_t loc;
  std::optional<std::string> text; // raw text
  std::optional<int> level;        // for heading, ul, ol
};

class BLexer {
public:
  explicit BLexer(const std::string &filepath);
  void b_tokenize(); // block tokenizer
  void print_tokens();
  std::vector<BToken> get_tokens();

private:
  std::vector<BToken> tokens;
  std::string creole_data;
  size_t pos;
  size_t loc;

  /*=== Printing Functions ===*/
  std::string token_to_string(BlockTokenType type);

  /*=== Reading Functions ===*/
  void read_heading();       // heaading with different levels
  void read_uli();           // unordered list item
  void read_oli();           // ordered list item
  void read_horizonalrule(); // horizontal rule
  void read_paragraph(); // normal paragraph lines
  void read_verbatim();      // verbatim block
  void read_blankline();

  /*=== Helper Functions ===*/
  inline bool end();
  void advance();
  char peek();
  char lookahead();
  inline bool is_newline();
  inline bool is_whites();
};

#endif // !B_LEXER_H
