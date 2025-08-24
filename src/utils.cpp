#include "utils.h"
#include <fstream>
#include <iostream>

void usage(const std::string &program) {
  std::cout << "Usage: " << program << " <input filepath>" << std::endl;
}

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

std::vector<std::string> chop_by_lines(const std::string &data) {
  std::vector<std::string> data_lines;
  std::string line;
  for (const char &c : data) {
    if (c == '\n') {
      data_lines.push_back(line);
      line.clear();
    } else {
      line += c;
    }
  }
  return data_lines;
}

Args parse_args(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    exit(1);
  }
  Args args;
  args.filename = argv[1];
  return args;
}
