#include "i_lexer.h"
#include <iostream>

std::vector<IToken> ILexer::tokenize(const std::string &input, size_t s_loc) {
  /* heating the engine vroom...vrooooom */
  heat_the_engine(input, s_loc);

  while (!end()) {
    char c = inline_data[pos];

    switch (curr_state) {
    case State::NORMAL:
      handle_normal_state(c);
      break;
    case State::IN_BOLD:
      handle_bold_state(c);
      break;
    case State::IN_ITALIC:
      handle_italic_state(c);
      break;
    case State::IN_LINK:
      handle_link_state(c);
      break;
    case State::IN_IMAGE:
      handle_image_state(c);
      break;
    case State::IN_VERBATIM:
      handle_verbatim_state(c);
      break;
    case State::ESCAPING:
      handle_escaping_state(c);
      break;
    default:
      std::cerr << "Unknown State" << std::endl;
      exit(1);
    }
    advance();

    if (c == '\n') {
    }
  }

  finalize_current_text();

  return i_tokens;
}

/*=== Printing ===*/
std::string ILexer::token_type_to_string(InlineTokenType type) {
  switch (type) {
  case InlineTokenType::TEXT:
    return "TEXT";
  case InlineTokenType::BOLD:
    return "BOLD";
  case InlineTokenType::ITALIC:
    return "ITALIC";
  case InlineTokenType::LINK:
    return "LINK";
  case InlineTokenType::IMAGE:
    return "IMAGE";
  case InlineTokenType::VERBATIM:
    return "VERBATIM";
  case InlineTokenType::ESCAPE:
    return "ESCAPE";
  case InlineTokenType::LINEBREAK:
    return "LINEBREAK";
  case InlineTokenType::ENDOF:
    return "ENDOF";
  default:
    return "UNKOWN";
  }
}

void ILexer::print_inline_tokens(const std::vector<IToken> &tokens) {
  for (const auto &t : tokens) {
    std::cout << "Type: " << token_type_to_string(t.type) << std::endl;
    std::cout << "Loc: " << t.loc << std::endl;

    if (t.content.has_value()) {
      std::cout << "Content: " << t.content.value() << std::endl;
    }

    if (t.url.has_value()) {
      std::cout << "URL: " << t.url.value() << std::endl;
    }

    if (!t.children.empty()) {
      std::cout << "Children: " << std::endl;
      print_inline_tokens(t.children);
    }
    std::cout << "----------------------------------------------" << std::endl;
  }
}

/*=== Processing Functions ===*/
void ILexer::handle_normal_state(char c) {
  switch (c) {
  case '*':
    if (lookahead() == '*') {
      finalize_current_text();
      curr_state = State::IN_BOLD;
      advance();
    } else {
      curr_text += c;
    }
    break;

  case '/':
    if (lookahead() == '/') {
      finalize_current_text();
      curr_state = State::IN_ITALIC;
      advance();
    } else {
      curr_text += c;
    }
    break;

  case '[':
    if (lookahead() == '[') {
      finalize_current_text();
      curr_state = State::IN_LINK;
      advance();
    } else {
      curr_text += c;
    }
    break;

  case '{':
    // if {{ -> image | if {{{ -> verbatim
    if (lookahead() == '{' && lookahead(2) != '{') {
      finalize_current_text();
      curr_state = State::IN_IMAGE;
      advance();
    } else if (lookahead() == '{' && lookahead(2) == '{') {
      finalize_current_text();
      curr_state = State::IN_VERBATIM;
      advance();
      advance();
    } else {
      curr_text += c;
    }
    break;

  case '~':
    finalize_current_text();
    curr_state = State::ESCAPING;
    break;

  case '\\':
    if (lookahead() == '\\') {
      finalize_current_text();
      add_token(InlineTokenType::LINEBREAK);
      advance();
    } else {
      curr_text += c;
    }
    break;

  default:
    curr_text += c;
    break;
  }
}

void ILexer::handle_bold_state(char c) {
  if (c == '*' && lookahead() == '*') {
    add_token(InlineTokenType::BOLD, curr_text);
    curr_text.clear();
    curr_state = State::NORMAL;
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_italic_state(char c) {
  if (c == '/' && lookahead() == '/') {
    add_token(InlineTokenType::ITALIC, curr_text);
    curr_text.clear();
    curr_state = State::NORMAL;
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_link_state(char c) {
  if (c == ']' && lookahead() == ']') {
    // parsing link content here
    std::string content = curr_text;
    curr_text.clear();

    // format -> [[url|text]] or [[url]]
    std::string url;
    std::string text;

    size_t pipe = content.find('|');
    if (pipe != std::string::npos) {
      url = content.substr(0, pipe);
      text = content.substr(pipe + 1);
    } else {
      url = content;
      text = content;
    }

    add_token(InlineTokenType::LINK, text, url);

    curr_state = State::NORMAL;
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_image_state(char c) {
  if (c == '}' && lookahead() == '}') {
    // parse image content
    std::string content = curr_text;
    curr_text.clear();

    // format -> {{url|alt}} or {{url}}
    std::string url;
    std::string alt;

    size_t pipe = content.find('|');
    if (pipe != std::string::npos) {
      url = content.substr(0, pipe);
      alt = content.substr(pipe + 1);
    } else {
      url = content;
      alt = content;
    }

    IToken img_token{InlineTokenType::IMAGE, loc, alt, url};
    i_tokens.push_back(img_token);

    curr_state = State::NORMAL;
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_verbatim_state(char c) {
  if (c == '}' && lookahead() == '}' && lookahead(2) == '}') {
    add_token(InlineTokenType::VERBATIM, curr_text);
    curr_text.clear();
    curr_state = State::NORMAL;
    advance();
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_escaping_state(char c) {
  // just take it as text
  curr_text += c;
  curr_state = State::NORMAL;
}

/*=== Helper Functions ===*/
void ILexer::heat_the_engine(const std::string &input, size_t s_loc) {
  i_tokens.clear();
  curr_text.clear();
  pos = 0;
  loc = s_loc;
  curr_state = State::NORMAL;
  inline_data = input;
}

bool ILexer::end() { return pos >= inline_data.size(); }

char ILexer::peek() {
  if (!end()) {
    return inline_data[pos];
  }
  return inline_data.back();
}

char ILexer::lookahead(size_t offset) {
  if (pos + offset < inline_data.size()) {
    return inline_data[pos + offset];
  }
  return inline_data.back();
}

void ILexer::advance() {
  if (!end()) {
    pos++;
  }
}

void ILexer::add_token(InlineTokenType type, std::optional<std::string> content,
                       std::optional<std::string> url) {
  IToken token(type, loc, content, url);
  i_tokens.push_back(token);
}

void ILexer::finalize_current_text() {
  if (!curr_text.empty()) {
    add_token(InlineTokenType::TEXT, curr_text);
    curr_text.clear();
  }
}
