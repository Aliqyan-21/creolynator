#include "migr_traversal.h"
#include "globals.h"

MIGRTraversal::MIGRTraversal(MIGRGraphLayer &layer) : layer_(layer) {
  _V_ << " [MigrTraversal] Traversal Interface Initialized." << std::endl;
}
