#!/usr/bin/bash

export API_SPEC=${PWD}/${1}
BASENAME=$(basename ${API_SPEC})
DIRNAME=$(dirname ${1})

export API_HOST_PATH=${DIRNAME#examples/}
export API_ADDR="https://${API_HOST_PATH}"
# export API_SPEC="${API_ADDR}/openapi.json"
export HEADER0="$(cat $(dirname ${API_SPEC})/.token)"
export HEADER1='x-tenant-Id: 5274c5fc-bcfb-4729-8c34-95565ba2070b'
export HEADER2='x-source-id: 4c861966eef847598027ed38243372f5'
export HEADER3="x-tenant-internal-id: 5274c5fc-bcfb-4729-8c34-95565ba2070b"

valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all --track-origins=yes -s \
./restfs -s -f /mnt/restfs
