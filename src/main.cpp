#include <iostream>
#include "utils.h"

int main(int argc, char *argv[]) {
  Args args = parse_args(argc, argv);
  std::cout << args.filename << std::endl;
  return 0;
}
