#!/usr/bin/bash

export API_SPEC=${PWD}/${1}
BASENAME=$(basename ${API_SPEC})
DIRNAME=$(dirname ${1})

export API_HOST_PATH=${DIRNAME#examples/}
export API_ADDR="https://${API_HOST_PATH}"
# export API_SPEC="${API_ADDR}/openapi.json"
export HEADER0="$(cat $(dirname ${API_SPEC})/.token)"

# valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all --track-origins=yes -s \
./bazel-bin/restfs -s -f /mnt/restfs
