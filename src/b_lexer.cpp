#include "b_lexer.h"
#include "error.h"
#include "i_lexer.h"
#include "utils.h"
#include <iostream>

B_Lexer::B_Lexer(const std::string &filepath) : pos(0), loc(1) {
  creole_data = read_creole_file(filepath);
}

/*=== Publicaly Exposed Functions ===*/
void B_Lexer::b_tokenize() {
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
      read_paragraphline();
    } else if (peek() == '{') {
      read_verbatim();
    } else if (std::isspace(peek())) {
      read_blankline();
    } else {
      advance();
    }
  }
  tokens.push_back({BlockTokenType::ENDOF, loc});
  f_tokenize();
}

void B_Lexer::f_tokenize() {
  for (const auto &token : tokens) {
    if (token.text.has_value()) {
      I_Lexer lex(token.text.value(), token.loc);
      lex.i_tokenize();
    }
  }
}

std::vector<B_Token> B_Lexer::get_tokens() {
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
std::string B_Lexer::token_to_string(BlockTokenType type) {
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
  case BlockTokenType::PARAGRAPHLINE:
    return "PARAGRAPHLINE";
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

void B_Lexer::print_tokens() {
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
void B_Lexer::read_heading() {
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

void B_Lexer::read_uli() {
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

void B_Lexer::read_oli() {
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

void B_Lexer::read_horizonalrule() {
  for (int i{0}; i < 4; ++i) {
    advance();
  }
  tokens.push_back({BlockTokenType::HORIZONTALRULE, loc});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void B_Lexer::read_paragraphline() {
  std::string line;
  while (!end() && !is_newline()) {
    line += peek();
    advance();
  }
  tokens.push_back({BlockTokenType::PARAGRAPHLINE, loc, trim(line)});
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

void B_Lexer::read_verbatim() {
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

void B_Lexer::read_blankline() {
  while (!end() && !is_newline()) {
    advance();
  }
  // let's just treat blankline as newline only
  tokens.push_back({BlockTokenType::NEWLINE, loc});
  advance(); // '\n'
}

/*=== Helper Functions ===*/
inline bool B_Lexer::end() { return pos >= creole_data.size(); }

void B_Lexer::advance() {
  if (!end()) {
    if (peek() == '\n') {
      loc++;
    }
    pos++;
  } else {
    throw I_LexerError("Unexpected end of tokens while advancing", loc);
  }
}

char B_Lexer::peek() {
  if (!end()) {
    return creole_data[pos];
  }
  throw B_LexerError("Unexpected end of tokens", loc);
}

inline bool B_Lexer::is_newline() { return peek() == '\n'; }

inline bool B_Lexer::is_whites() {
  return (peek() != '\n' && std::isspace(peek()));
}
