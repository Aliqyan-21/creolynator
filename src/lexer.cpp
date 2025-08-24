#include "lexer.h"
#include "error.h"
#include "utils.h"
#include <iostream>

Lexer::Lexer(const std::string &filepath) : pos(0), loc(1) {
  creole_data = read_creole_file(filepath);
}

/*=== Publicaly Exposed Functions ===*/
void Lexer::tokenize() {
  while (!end()) {
    if (peek() == '=') {
      read_heading();
    } else if (peek() == '*') {
      read_uli();
    } else if (peek() == '#') {
      read_oli();
    } else if (peek() == '-') {
      read_horizonalrule();
    } else {
      advance();
    }
  }
  tokens.push_back({BlockTokenType::ENDOF, loc});
}

std::vector<Token> Lexer::get_tokens() {
  if (tokens.empty() || tokens.size() == 1) {
    throw LexerError("Tried to call get_tokens function without populating the "
                     "tokens or running lexer on an empty file",
                     0);
    return {};
  }
  return tokens;
}

/*=== Printing Functions ===*/
std::string Lexer::token_to_string(BlockTokenType type) {
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
  case BlockTokenType::ENDOF:
    return "ENDOF";
    break;
  default:
    return "UNKNOWN";
    break;
  }
}

void Lexer::print_tokens() {
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
void Lexer::read_heading() {
  int level{0};
  while (peek() == '=') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && peek() != '\n') {
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

void Lexer::read_uli() {
  int level{0};
  while (peek() == '*') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && peek() != '\n') {
    text += peek();
    advance();
  }
  tokens.push_back({BlockTokenType::ULISTITEM, loc, trim(text), level});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void Lexer::read_oli() {
  int level{0};
  while (peek() == '#') {
    level++;
    advance();
  }

  std::string text;
  while (!end() && peek() != '\n') {
    text += peek();
    advance();
  }
  tokens.push_back({BlockTokenType::OLISTITEM, loc, trim(text), level});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void Lexer::read_horizonalrule() {
  for (int i{0}; i < 4; ++i) {
    advance();
  }
  tokens.push_back({BlockTokenType::HORIZONTALRULE, loc});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

/*=== Helper Functions ===*/
inline bool Lexer::end() { return pos >= creole_data.size(); }

void Lexer::advance() {
  if (!end()) {
    if (peek() == '\n') {
      loc++;
    }
    pos++;
  }
}

char Lexer::peek() {
  if (!end()) {
    return creole_data[pos];
  }
  throw LexerError("Unexpected end of tokens", loc);
}

inline bool Lexer::is_newline() { return peek() == '\n'; }

inline bool Lexer::is_whites() {
  return (peek() != '\n' && std::isspace(peek()));
}
