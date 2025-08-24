#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

struct Args {
  std::string filename;
};

void usage(void);
std::string read_creole_file(const std::string &filepath);
std::vector<std::string> chop_by_lines(const std::string &data);
Args parse_args(int argc, char *argv[]);

#endif // !UTILS_H
