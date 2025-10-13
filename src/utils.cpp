#include "utils.h"
#include <fstream>
#include <iostream>

/* prints usage info on console */
void usage(const std::string &program) {
  std::cout << "Usage: " << program << " <input filepath>" << std::endl;
}

/* opens file and reads it's content into a string and returns it */
std::string read_creole_file(const std::string &filepath) {
  std::ifstream infile(filepath);
  if (!infile) {
    std::cerr << "File not found: " << filepath << std::endl;
    exit(1);
  }
  std::string content((std::istreambuf_iterator<char>(infile)),
                      std::istreambuf_iterator<char>());
  return content;
}

/* parses the cli arguments, stores them in Args struct and returns it */
Args parse_args(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    exit(1);
  }
  Args args;
  args.filename = argv[1];
  return args;
}

/* trims whitespace from left direction of string and returns it */
std::string ltrim(const std::string &str) {
  size_t i = 0;
  while (i < str.size() && std::isspace(str[i])) {
    i++;
  }
  return str.substr(i);
}

/* trims whitespace from right direction of string and returns it */
std::string rtrim(const std::string &str) {
  size_t j = str.size();
  while (j > 0 && std::isspace(str[j - 1])) {
    j--;
  }
  return str.substr(0, j);
}

/* trims whitespace from left and right
both direction of string and returns it */
std::string trim(const std::string &str) { return ltrim(rtrim(str)); }
