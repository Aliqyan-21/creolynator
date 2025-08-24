#ifndef UTILS_H
#define UTILS_H

#include <string>

struct Args {
  std::string filename;
};

void usage(void);
std::string read_creole_file(const std::string &filepath);
Args parse_args(int argc, char *argv[]);
std::string ltrim(const std::string &str);
std::string rtrim(const std::string &str);
std::string trim(const std::string &str);

#endif // !UTILS_H
