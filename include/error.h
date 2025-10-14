#ifndef ERROR_H
#define ERROR_H

#include <exception>
#include <string>

class CNError : public std::exception {
public:
  std::string msg;
  size_t loc;

  CNError(const std::string &msg, size_t loc);

  virtual std::string format() const;
};

class B_LexerError : public CNError {
public:
  explicit B_LexerError(const std::string &msg, size_t loc);
};

class I_LexerError : public CNError {
public:
  explicit I_LexerError(const std::string &msg, size_t loc);
};

class MIGRError : public std::exception {
  std::string msg;
  size_t line_num;
  std::string recovery_action;

public:
  enum class Severity { WARNING, ERROR, FATAL };

  MIGRError(const std::string &msg, size_t line,
            const std::string &action = "");

  const char *what() const noexcept override;

  Severity get_severity() const;
  bool can_recover() const;
  size_t get_line() const;
};

#endif // !ERROR_H
