#!bin/bash

printf "5\r\nHello\r\n5\r\nWorld\r\n0\r\n\r\n" > chunked_input.txt

curl -v -X POST http://localhost:8080/uploads/chunktest.txt \
  -H "Transfer-Encoding: chunked" \
  -H "Content-Type: text/plain" \
  --data-binary @chunked_input.txt
