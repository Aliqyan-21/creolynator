#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>

inline bool verbose = true;

/* output when verbose is true */
#define _V_                                                                    \
  if (verbose)                                                                 \
  std::cerr << "[VERBOSE] "

/* output always as cerr output stream */
#define SPEAK std::cerr << "[INTERNAL] "

#endif //! GLOBALS_H
