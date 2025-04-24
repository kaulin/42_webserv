#!/bin/bash

echo -n "HulloWorld" > post_input.txt

curl -v -X POST http://localhost:8080/uploads/posttest.txt \
  -H "Content-Type: text/plain" \
  --data-binary @post_input.txt

rm post_input.txt
