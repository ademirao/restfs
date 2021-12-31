#include "path.h"

int main(int argc, char *argv[]) {
  const path::Path path = "/root/ademirao/{}/{:32}/{ref}/{ref:}/{ref:32}/daniela/{ref2:33}/"
                    "{ref3}get.json";
  const path::Path canonical =
      "/root/ademirao/{}/{}/{ref}/{ref}/{ref}/daniela/{ref2}/{ref3}get.json";
  LOG(INFO) << "Canonicalizing... " << path << " expected " << canonical;
  path::RefValueMap map;
  auto result = path::utils::PathToRefValueMap(path, &map);
  CHECK(map[""] == "32");
  LOG(INFO) << "Canonical" << result;
  if (result == canonical) {
    LOG(INFO) << "Success";
    return 0;
  }
  LOG(FATAL) << "Got " << result << " expected " << canonical;
  return 1; // unreachable
}