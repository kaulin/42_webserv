#!/bin/bash

echo -n "HulloWorld" > post_input.txt

for num in $(seq 1 5000); do
  for port in {8080..8082}; do
    echo "Testing port $port..."
    curl -v -X POST http://localhost:$port/uploads/posttest_$port.txt \
      -H "Content-Type: text/plain" \
      --data-binary @post_input.txt
  done
done

rm post_input.txt
