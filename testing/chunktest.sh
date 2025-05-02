#!/bin/bash

printf "5\r\nHillo\r\n5\r\nWorld\r\n0\r\n\r\n" > chunked_input.txt

for num in $(seq 1 10); do
  for port in {8080..8097}; do
    echo "Testing port $port..."
    curl -v -X POST http://localhost:$port/uploads/chunktest_$port.txt \
      -H "Transfer-Encoding: chunked" \
      -H "Content-Type: text/plain" \
      --data-binary @chunked_input.txt
  done
done

rm chunked_input.txt
