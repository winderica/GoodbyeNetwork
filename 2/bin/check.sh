#!/bin/bash

appname='./rdt'
inputname='../test/input.txt'
outputname='../test/output.txt'
resultname='../test/result.txt'

items=("Stop-wait" "GBN" "SR" "TCP")

for j in {3..3}; do
  echo "Checking ${items[j]}"
  echo "================================"
  for i in {1..1000}; do
    echo "Round $i"
    ./$appname $j >$resultname 2>&1
    diff $inputname $outputname &&
      echo "Round $i succeeded" ||
      echo "Round $i failed"
  done
  echo "================================"
done
