#include "path.h"

int main(int argc, char *argv[]) {
  path::Path path = "/root/ademirao/{}/{=32}/{ref=}/{ref=32}/daniela/{ref2=33}";
  path::Path canonical = "/root/ademirao/{}/{}/{ref}/{ref}/daniela/{ref2}";
  LOG(INFO) << "Canonicalizing... " << path << " expected " << canonical;
  auto result = path::utils::CanonicalizeRefs(path);
  LOG(INFO) << "Canonical" << result;
  if (result == canonical) {
    LOG(INFO) << "Success";
    return 0;
  }
  LOG(FATAL) << "Got " << result << " expected " << canonical;
  return 1; // unreachable
}