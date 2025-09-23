#ifndef I_LEXER_H
#define I_LEXER_H

#include <optional>
#include <string>
#include <vector>

enum class InlineTokenType {
  TEXT,
  BOLD,
  ITALIC,
  LINK,
  IMAGE,
  VERBATIM,
  ESCAPE,
  LINEBREAK,
  ENDOF,
};

struct IToken {
  InlineTokenType type;
  size_t loc;
  std::optional<std::string> content;
  std::optional<std::string> url; // for links and images
  std::vector<IToken> children;   // for nested formatting
};

/*
FSM (Finite State Machine) handling of inline tokens
- this makes the lexer very independent of tokens.
- telling the lexer what to do, not how to do it.
- it's figuring "how to" on it's own, according to current state.
*/
class ILexer {
public:
  ILexer() = default;
  std::vector<IToken> tokenize(const std::string &input, size_t s_loc = 0);

  /*=== Printing ===*/
  static std::string token_type_to_string(InlineTokenType type);
  static void print_inline_tokens(const std::vector<IToken> &tokens);

private:
  enum class State {
    NORMAL,      // text with no formatting
    IN_BOLD,     // bold text
    IN_ITALIC,   // italic text
    IN_LINK,     // link in text
    IN_IMAGE,    // image link in text
    IN_VERBATIM, // unformatted inline text
    ESCAPING, // ~ : escapes all formatting, escaping characters are used as is
  };

  /* State Variables */
  std::vector<IToken> i_tokens;
  std::string curr_text;
  size_t pos;
  size_t loc;
  State curr_state;
  std::string inline_data;

  /*=== Processing Functions ===*/
  void handle_normal_state(char c);
  void handle_bold_state(char c);
  void handle_italic_state(char c);
  void handle_link_state(char c);
  void handle_image_state(char c);
  void handle_verbatim_state(char c);
  void handle_escaping_state(char c);

  /*=== Helper Functions ===*/
  void heat_the_engine(const std::string &input, size_t s_loc);

  bool end();
  char peek();
  char lookahead(size_t offset = 1);
  void advance();

  void add_token(InlineTokenType type,
                 std::optional<std::string> content = std::nullopt,
                 std::optional<std::string> url = std::nullopt);
  void finalize_current_text();
};

#endif //! I_LEXER_H
