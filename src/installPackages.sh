#!/bin/bash

FILE="packages"

while IFS= read -r line
do
  apt-get install -y "$line"
done < $FILE