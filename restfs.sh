#!/usr/bin/bash

export API_HOST="login.swiftkanban.com"
export API_ADDR="https://${API_HOST}/restapi"
# export API_SPEC="${PWD}/examples/${API_HOST}-restapi.openapi.json"
export API_SPEC="${API_ADDR}/openapi.json"
export API_TOKEN="AuthorizationToken: $(cat .token)"
export API_CACHE_TTL=0

./restfs -s -f /mnt/restfs
