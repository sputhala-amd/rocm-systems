#include "common/environment.hpp"
#include "common/static_object.hpp"

extern "C" {
void
rocprofsys_attach(size_t pid) __attribute__((visibility("default")));

void
detach() __attribute__((visibility("default")));
}