#ifndef ERROR_H
#define ERROR_H

#include <exception>
#include <string>

class B_LexerError : public std::exception {
public:
  explicit B_LexerError(const std::string &msg, size_t loc);
  virtual const char *what() const noexcept override;

private:
  std::string msg;
  std::string formatted;
  size_t loc;
};

class I_LexerError : public std::exception {
public:
  explicit I_LexerError(const std::string &msg, size_t loc);
  virtual const char *what() const noexcept override;

private:
  std::string msg;
  std::string formatted;
  size_t loc;
};

#endif // !ERROR_H
