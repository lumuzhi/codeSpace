#!/bin/bash

while IFS= read -r line; do
  echo "Input: $line"
done < queue_main.c