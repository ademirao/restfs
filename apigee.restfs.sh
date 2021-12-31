#!/usr/bin/bash

export API_HOST="apigee.googleapis.com"
export API_ADDR="https://${API_HOST}"
# export API_SPEC="${API_ADDR}/openapi.json"
export API_SPEC="${PWD}/examples/apigee.openapi.json"
export API_TOKEN="AuthorizationToken: $(cat .token.apigee)"
export API_CACHE_TTL=0

# valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all --track-origins=yes -s \
./restfs -s -f /mnt/restfs
