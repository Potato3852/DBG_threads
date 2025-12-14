#!/bin/bash

THREADS=${1:-4}

mkdir -p build results

echo "Starting with $THREADS threads..."

python3 python/controller.py --threads $THREADS

echo ""