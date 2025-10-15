#include "i_lexer.h"
#include "globals.h"
#include "utils.h"
#include <iostream>

std::vector<IToken> ILexer::tokenize(const std::string &input, size_t s_loc) {
  /* heating the engine vroom...vrooooom */
  heat_the_engine(input, s_loc);

  _V_ << " [ILexer] Starting Inline Tokenization..." << std::endl;
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
    case State::IN_ESCAPE:
      handle_escape_state(c);
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

  _V_ << " [ILexer] Inline Tokenization Ended." << std::endl;
  return i_tokens;
}

/*=== Formatting Location ===*/
void ILexer::start_formatting() {
  fmt_pos = pos;
  fmt_loc = loc;
  fmt_stack.push_back(loc);
}

size_t ILexer::get_format_start_loc() {
  if (fmt_stack.empty()) {
    return loc;
  }
  return fmt_stack.back();
}

void ILexer::end_formatting() {
  if (!fmt_stack.empty()) {
    fmt_stack.pop_back();
  }
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
  }
}

/*=== Handling Children ===*/
std::vector<IToken> ILexer::recursive_tokenize(const std::string &input,
                                               size_t s_loc) {
  ILexer nested_lexer;
  return nested_lexer.tokenize(input, s_loc);
}

/*=== Processing Functions ===*/
void ILexer::handle_normal_state(char c) {
  _V_ << " [ILexer] Current State: NORMAL." << std::endl;
  switch (c) {
  case '*':
    if (lookahead() == '*') {
      finalize_current_text();
      start_formatting();
      curr_state = State::IN_BOLD;
      advance();
    } else {
      curr_text += c;
    }
    break;

  case '/':
    if (lookahead() == '/') {
      // immediately after "http:" or "ftp:"
      bool after_url_protocol = false;
      if (curr_text.size() >= 5) {
        std::string last5 = curr_text.substr(curr_text.size() - 5);
        if (last5 == "http:" || last5 == "HTTP:") {
          after_url_protocol = true;
        }
      }
      if (!after_url_protocol && curr_text.size() >= 4) {
        std::string last4 = curr_text.substr(curr_text.size() - 4);
        if (last4 == "ftp:" || last4 == "FTP:") {
          after_url_protocol = true;
        }
      }

      if (after_url_protocol) {
        curr_text += c;
      } else {
        // start italic formatting.
        finalize_current_text();
        start_formatting();
        curr_state = State::IN_ITALIC;
        advance();
      }
    } else {
      curr_text += c;
    }
    break;

  case '[':
    if (lookahead() == '[') {
      finalize_current_text();
      start_formatting();
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
      start_formatting();
      curr_state = State::IN_IMAGE;
      advance();
    } else if (lookahead() == '{' && lookahead(2) == '{') {
      finalize_current_text();
      start_formatting();
      curr_state = State::IN_VERBATIM;
      advance(2);
    } else {
      curr_text += c;
    }
    break;

  case '~': {
    finalize_current_text();
    curr_state = State::IN_ESCAPE;
    break;
  }

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
  _V_ << " [ILexer] Current State: IN_BOLD." << std::endl;
  if (c == '*' && lookahead() == '*') {
    if (!curr_text.empty()) {
      // recursively tokenizing the content for nested formatting
      auto nested_tokens = recursive_tokenize(curr_text, fmt_loc);

      IToken bold_token(InlineTokenType::BOLD, loc);
      bold_token.children = nested_tokens;
      i_tokens.push_back(bold_token);

      curr_text.clear();
    }
    end_formatting();
    curr_state = State::NORMAL;
    advance();
  } else if (c == '~') {
    finalize_current_text();
    curr_state = State::IN_ESCAPE;
  } else {
    curr_text += c;
  }
}

void ILexer::handle_italic_state(char c) {
  _V_ << " [ILexer] Current State: IN_ITALIC." << std::endl;
  if (c == '/' && lookahead() == '/') {
    if (!curr_text.empty()) {
      auto nested_tokens = recursive_tokenize(curr_text, fmt_loc);

      IToken italic_token(InlineTokenType::ITALIC, loc);
      italic_token.children = nested_tokens;
      i_tokens.push_back(italic_token);

      curr_text.clear();
    }
    end_formatting();
    curr_state = State::NORMAL;
    advance();
  } else if (c == '~') {
    finalize_current_text();
    curr_state = State::IN_ESCAPE;
  } else {
    curr_text += c;
  }
}

void ILexer::handle_link_state(char c) {
  _V_ << " [ILexer] Current State: IN_LINK." << std::endl;
  if (c == ']' && lookahead() == ']') {
    // parsing link content here
    std::string content = curr_text;
    curr_text.clear();

    // format -> [[url|text]] or [[url]]
    std::string url;
    std::string text;

    size_t pipe = content.find('|');
    if (pipe != std::string::npos) {
      url = trim(content.substr(0, pipe));
      text = trim(content.substr(pipe + 1));

      IToken link_token(InlineTokenType::LINK, loc, text, url);
      i_tokens.push_back(link_token);
    } else {
      url = trim(content);

      IToken link_token(InlineTokenType::LINK, loc, url, url);
      i_tokens.push_back(link_token);
    }

    end_formatting();
    curr_state = State::NORMAL;
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_image_state(char c) {
  _V_ << " [ILexer] Current State: IN_IMAGE." << std::endl;
  if (c == '}' && lookahead() == '}') {
    // parse image content
    std::string content = curr_text;
    curr_text.clear();

    // format -> {{url|alt}} or {{url}}
    std::string url;
    std::string alt;

    size_t pipe = content.find('|');
    if (pipe != std::string::npos) {
      url = trim(content.substr(0, pipe));
      alt = trim(content.substr(pipe + 1));
    } else {
      url = trim(content);
      alt = "";
    }

    IToken img_token(InlineTokenType::IMAGE, loc, alt, url);
    i_tokens.push_back(img_token);

    end_formatting();
    curr_state = State::NORMAL;
    advance();
  } else {
    curr_text += c;
  }
}

void ILexer::handle_verbatim_state(char c) {
  _V_ << " [ILexer] Current State: IN_VERBATIM." << std::endl;
  if (c == '}' && lookahead() == '}' && lookahead(2) == '}') {
    add_token(InlineTokenType::VERBATIM, curr_text);
    curr_text.clear();
    end_formatting();
    curr_state = State::NORMAL;
    advance(2);
  } else {
    curr_text += c;
  }
}

void ILexer::handle_escape_state(char c) {
  _V_ << " [ILexer] Current State: IN_ESCAPE." << std::endl;
  curr_text += c;
  curr_state = State::NORMAL;
}

/*=== Helper Functions ===*/
void ILexer::heat_the_engine(const std::string &input, size_t s_loc) {
  _V_ << " [ILexer] Heating The Engine..." << std::endl;
  i_tokens.clear();
  curr_text.clear();
  pos = 0;
  loc = s_loc;
  curr_state = State::NORMAL;
  inline_data = input;
  fmt_pos = 0;
  fmt_loc = s_loc;
  fmt_stack.clear();
  _V_ << " [ILexer] Engine Heated." << std::endl;
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

void ILexer::advance(size_t offset) {
  if (pos + offset <= inline_data.size()) {
    pos += offset;
  }
}

void ILexer::add_token(InlineTokenType type, std::optional<std::string> content,
                       std::optional<std::string> url) {
  IToken token(type, loc, content, url);
  i_tokens.push_back(token);
}

void ILexer::finalize_current_text() {
  _V_ << " [ILexer] Finalizing Current State." << std::endl;
  if (!curr_text.empty()) {
    add_token(InlineTokenType::TEXT, curr_text);
    curr_text.clear();
  }
}
