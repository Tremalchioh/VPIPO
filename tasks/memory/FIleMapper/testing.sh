#!/bin/bash

set -e

echo "================= Compilation: Compiling supaBigSort.cpp ================="
g++ -std=c++17 -o supaBigSort supaBigSort.cpp
echo "✅ Compilation successful!"

echo "================= Test 1: Generate Random Data ================="
./supaBigSort --gen randData.bin 1000000
./supaBigSort --check randData.bin
./supaBigSort randData.bin 100
./supaBigSort --check randData.bin

echo "================= Test 2: Already Sorted Data ================="
./supaBigSort --gen sorted_data.bin 1000000 sorted
./supaBigSort --check sorted_data.bin
./supaBigSort sorted_data.bin

echo "================= Test 3: Empty File ================="
touch null.bin
./supaBigSort --check null.bin
./supaBigSort null.bin

echo "================= Test 4: Single-Element File ================="
echo -n -e "\x01\x00\x00\x00\x00\x00\x00\x00" > oneEl.bin
./supaBigSort --check oneEl.bin
./supaBigSort oneEl.bin

echo "================= Test 5: Two-Element Unsorted File ================="
echo -n -e "\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" > small.bin
./supaBigSort --check small.bin
./supaBigSort small.bin
./supaBigSort --check small.bin

echo "================= Test 6: Large Dataset (10 million numbers) ================="
./supaBigSort --gen bigAssData.bin 10000000
./supaBigSort --check bigAssData.bin
./supaBigSort bigAssData.bin 100
./supaBigSort --check bigAssData.bin

echo "================= Test 7: Invalid Inputs ================="
./supaBigSort sorted_data.bin abc
./supaBigSort

echo "✅ All tests completed successfully!"
