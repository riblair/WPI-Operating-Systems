#! /bin/bash

WORKING_DIR=$(pwd)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [[ "$WORKING_DIR" != "$SCRIPT_DIR" ]]; then
  echo "Please rerun this script from: $SCRIPT_DIR"
  exit 1
fi

echo "PART 1:"
cd test-part1
./test_part1.sh $*
cd ..

echo "PART 3:"
cd test-part3
./test_part3.sh $*
cd ..
