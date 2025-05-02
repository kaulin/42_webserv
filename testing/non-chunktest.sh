#!/bin/bash

echo -n "HulloWorld" > post_input.txt

for port in {8080..8097}; do
  echo "Testing port $port..."
  curl -v -X POST http://localhost:$port/uploads/posttest_$port.txt \
    -H "Content-Type: text/plain" \
    --data-binary @post_input.txt
done

rm post_input.txt
