#!/bin/bash

FILE="packages"

while IFS= read -r line
do
  apt install -y "$line"
done < $FILE