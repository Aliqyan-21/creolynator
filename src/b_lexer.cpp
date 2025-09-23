#include "b_lexer.h"
#include "error.h"
#include "utils.h"
#include <iostream>

BLexer::BLexer(const std::string &filepath) : pos(0), loc(1) {
  creole_data = read_creole_file(filepath);
}

/*=== Publicaly Exposed Functions ===*/
void BLexer::b_tokenize() {
  while (!end()) {
    if (peek() == '=') {
      read_heading();
    } else if (peek() == '*') {
      read_uli();
    } else if (peek() == '#') {
      read_oli();
    } else if (peek() == '-') {
      read_horizonalrule();
    } else if (peek() == '{' && lookahead() == '{' && lookahead(2) == '{') {
      read_verbatim();
    } else if (peek() == '{' && lookahead() == '{') {
      read_image();
    } else if (std::isspace(peek())) {
      read_blankline();
    } else {
      read_paragraph();
    }
  }
  tokens.push_back({BlockTokenType::ENDOF, loc});
}

std::vector<BToken> BLexer::get_tokens() {
  if (tokens.empty() || tokens.size() == 1) {
    throw B_LexerError(
        "Tried to call get_tokens function without populating the "
        "tokens or running lexer on an empty file",
        0);
    return {};
  }
  return tokens;
}

/*=== Printing Functions ===*/
std::string BLexer::token_to_string(BlockTokenType type) {
  switch (type) {
  case BlockTokenType::HEADING:
    return "HEADING";
  case BlockTokenType::NEWLINE:
    return "NEWLINE";
  case BlockTokenType::ULISTITEM:
    return "ULISTITEM";
  case BlockTokenType::OLISTITEM:
    return "OLISTITEM";
  case BlockTokenType::HORIZONTALRULE:
    return "HORIZONTALRULE";
  case BlockTokenType::PARAGRAPH:
    return "PARAGRAPH";
  case BlockTokenType::VERBATIMBLOCK:
    return "VERBATIMBLOCK";
  case BlockTokenType::IMAGE:
    return "IMAGE";
  case BlockTokenType::ENDOF:
    return "ENDOF";
  default:
    return "UNKNOWN";
  }
}

void BLexer::print_tokens() {
  for (const auto &t : tokens) {
    std::cout << "Type: " << token_to_string(t.type) << std::endl;
    std::cout << "Loc: " << t.loc << std::endl;
    if (t.text.has_value()) {
      std::cout << "Text: " << t.text.value() << std::endl;
    }
    if (t.level.has_value()) {
      std::cout << "Level: " << t.level.value() << std::endl;
    }
    if (!t.i_tokens.empty()) {
      std::cout << "========INLINE TOKENS=====" << std::endl;
      ILexer::print_inline_tokens(t.i_tokens);
    }
    std::cout << "--------------------------------------------" << std::endl;
  }
}

/*=== Reading Functions ===*/
void BLexer::read_heading() {
  int level{0};
  while (peek() == '=') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && !is_newline()) {
    if (peek() == '=') {
      advance();
    } else {
      text += peek();
      advance();
    }
  }
  tokens.push_back({BlockTokenType::HEADING, loc, trim(text), level});
  advance(); // '\n'
}

void BLexer::read_uli() {
  int level{0};
  while (peek() == '*') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && !is_newline()) {
    text += peek();
    advance();
  }
  tokens.push_back({BlockTokenType::ULISTITEM, loc, trim(text), level});
  advance(); // '\n'
}

void BLexer::read_oli() {
  int level{0};
  while (peek() == '#') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && !is_newline()) {
    text += peek();
    advance();
  }
  tokens.push_back({BlockTokenType::OLISTITEM, loc, trim(text), level});
  advance(); // '\n'
}

void BLexer::read_horizonalrule() {
  for (int i{0}; i < 4; ++i) {
    advance();
  }
  tokens.push_back({BlockTokenType::HORIZONTALRULE, loc});
  advance(); // '\n'
}

void BLexer::read_paragraph() {
  std::string text;
  size_t start_loc = loc;
  while (!end()) {
    if (is_special()) {
      break;
    }

    while (!end() && !is_newline()) {
      text += peek();
      advance();
    }

    if (is_newline()) {
      advance();
    }
    text += '\n';
  }

  tokens.push_back({BlockTokenType::PARAGRAPH, start_loc, trim(text)});
}

void BLexer::read_verbatim() {
  size_t _loc = loc;
  int depth{1};
  advance(3); // {

  std::string text;
  while (!end() && depth > 0) {
    if (peek() == '{' && lookahead() == '{' && lookahead(2) == '{') {
      depth++;
      text += "{{{";
      advance(3); // {
    } else if (peek() == '}' && lookahead() == '}' && lookahead(2) == '}') {
      depth--;
      if (depth == 0) {
        advance(3); // {
        break;
      } else {
        text += "}}}";
        advance(3); // {
      }
    } else {
      text += peek();
      advance();
    }
  }
  tokens.push_back({BlockTokenType::VERBATIMBLOCK, _loc, text});

  while (!end() && !is_newline()) {
    advance();
  }
  if (!end() && is_newline()) {
    advance(); // \n
  }
}

void BLexer::read_blankline() {
  while (!end() && !is_newline()) {
    advance();
  }
  // let's just treat blankline as newline only
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void BLexer::read_image() {
  advance(2); // {

  std::string text;
  while (!end() && !(peek() == '}' && lookahead() == '}')) {
    text += peek();
    advance();
  }
  advance(2); // }

  tokens.push_back({BlockTokenType::IMAGE, loc, trim(text)});
}

/*=== Inline ===*/
void BLexer::process_inline_tokens() {
  ILexer i_lexer;

  // todo: skip verbatim
  for (auto &t : tokens) {
    if (t.text.has_value() && t.type != BlockTokenType::VERBATIMBLOCK) {
      t.i_tokens = i_lexer.tokenize(t.text.value(), t.loc);
    }
  }
}

/*=== Helper Functions ===*/
inline bool BLexer::end() { return pos >= creole_data.size(); }

void BLexer::advance(size_t offset) {
  if (!end() && pos + offset <= creole_data.size()) {
    for (size_t i{0}; i < offset; i++) {
      if (creole_data[pos + i] == '\n') {
        loc++;
      }
    }
    pos += offset;
  } else {
    throw B_LexerError("Unexpected end of tokens while advancing" +
                           std::to_string(offset) + " steps",
                       loc);
  }
}

char BLexer::peek() {
  if (!end()) {
    return creole_data[pos];
  }
  throw B_LexerError("Unexpected end of tokens", loc);
}

char BLexer::lookahead(size_t offset) {
  if (!end() && pos + offset < creole_data.size()) {
    return creole_data[pos + offset];
  }
  throw B_LexerError("Unexpected end of tokens", loc);
}

inline bool BLexer::is_newline() { return peek() == '\n'; }

inline bool BLexer::is_whites() {
  return (peek() != '\n' && std::isspace(peek()));
}

inline bool BLexer::is_special() {
  return (peek() == '=' || peek() == '*' || peek() == '#' || peek() == '-' ||
          (peek() == '{' && lookahead() == '{' && lookahead(2) == '{') ||
          (peek() == '{' && lookahead() == '{') || std::isspace(peek()));
}
