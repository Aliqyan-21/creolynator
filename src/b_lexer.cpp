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
    } else if (std::isalnum(peek())) {
      read_paragraph();
    } else if (peek() == '{') {
      read_verbatim();
    } else if (std::isspace(peek())) {
      read_blankline();
    } else {
      advance();
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
    break;
  case BlockTokenType::NEWLINE:
    return "NEWLINE";
    break;
  case BlockTokenType::ULISTITEM:
    return "ULISTITEM";
    break;
  case BlockTokenType::OLISTITEM:
    return "OLISTITEM";
    break;
  case BlockTokenType::HORIZONTALRULE:
    return "HORIZONTALRULE";
    break;
  case BlockTokenType::PARAGRAPH:
    return "PARAGRAPH";
    break;
  case BlockTokenType::VERBATIMBLOCK:
    return "VERBATIMBLOCK";
    break;
  case BlockTokenType::ENDOF:
    return "ENDOF";
    break;
  default:
    return "UNKNOWN";
    break;
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
  tokens.push_back({BlockTokenType::NEWLINE, loc});
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
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void BLexer::read_oli() {
  int level{0};
  while (peek() == '#') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && is_newline()) {
    text += peek();
    advance();
  }
  tokens.push_back({BlockTokenType::OLISTITEM, loc, trim(text), level});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void BLexer::read_horizonalrule() {
  for (int i{0}; i < 4; ++i) {
    advance();
  }
  tokens.push_back({BlockTokenType::HORIZONTALRULE, loc});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void BLexer::read_paragraph() {
  std::string line;
  size_t start_loc = loc;
  while (!end()) {
    if (!std::isalnum(peek())) {
      break;
    }

    while (!end() && !is_newline()) {
      line += peek();
      advance();
    }

    if (is_newline()) {
      advance();
    }
  }

  tokens.push_back({BlockTokenType::PARAGRAPH, start_loc, trim(line)});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
}

void BLexer::read_verbatim() {
  size_t _loc = loc;
  for (int i{0}; i < 3; ++i) {
    advance(); // {
  }
  while (!end() && !is_newline()) {
    advance(); // skip for now
  }
  advance(); //\n
  std::string text;
  while (!end() && peek() != '}') {
    text += peek();
    advance();
  }
  for (int i{0}; i < 3; ++i) {
    advance(); // }
  }
  tokens.push_back({BlockTokenType::VERBATIMBLOCK, _loc, text});
  while (!end() && !is_newline()) {
    advance(); // skip for now
  }
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void BLexer::read_blankline() {
  while (!end() && !is_newline()) {
    advance();
  }
  // let's just treat blankline as newline only
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

/*=== Helper Functions ===*/
inline bool BLexer::end() { return pos >= creole_data.size(); }

void BLexer::advance() {
  if (!end()) {
    if (peek() == '\n') {
      loc++;
    }
    pos++;
  } else {
    throw B_LexerError("Unexpected end of tokens while advancing", loc);
  }
}

char BLexer::peek() {
  if (!end()) {
    return creole_data[pos];
  }
  throw B_LexerError("Unexpected end of tokens", loc);
}

inline bool BLexer::is_newline() { return peek() == '\n'; }

inline bool BLexer::is_whites() {
  return (peek() != '\n' && std::isspace(peek()));
}
