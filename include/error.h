#ifndef ERROR_H
#define ERROR_H

#include <exception>
#include <string>

class LexerError : public std::exception {
public:
  explicit LexerError(const std::string &msg, size_t loc);
  virtual const char *what() const noexcept override;

private:
  std::string msg;
  std::string formatted;
  size_t loc;
};

#endif // !ERROR_H
