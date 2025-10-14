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

class MIGRError : public CNError {
public:
  explicit MIGRError(const std::string &msg, size_t loc);
};

#endif // !ERROR_H
