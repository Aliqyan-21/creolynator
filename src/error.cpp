#include "error.h"
#include <sstream>

CNError::CNError(const std::string &msg, size_t loc) : msg(msg), loc(loc) {}

std::string CNError::format() const {
  std::stringstream ss;
  ss << "Error at " << loc << " : " << what();
  return ss.str();
}

B_LexerError::B_LexerError(const std::string &msg, size_t loc)
    : CNError("Block Lexer: " + msg, loc) {}

I_LexerError::I_LexerError(const std::string &msg, size_t loc)
    : CNError("Inline Lexer: " + msg, loc) {}

MIGRError::MIGRError(const std::string &msg, size_t line,
                     const std::string &action)
    : msg(msg), line_num(line), recovery_action(action) {}

const char *MIGRError::what() const noexcept { return msg.c_str(); }

MIGRError::Severity MIGRError::get_severity() const {
  if (msg.find("FATAL") != std::string::npos) {
    return Severity::FATAL;
  }
  if (msg.find("ERROR") != std::string::npos) {
    return Severity::ERROR;
  }
  return Severity::WARNING;
}

bool MIGRError::can_recover() const { return !recovery_action.empty(); }

size_t MIGRError::get_line() const { return line_num; }
