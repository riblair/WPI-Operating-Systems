#! /bin/bash

# DO NOT MODIFY #
#
# This script is used to measure the time taken for the (parallel, serial) zip versions
# currently runs each test case 10 times ($repeat) to get a stable speedup/slowdown measurement
#

measure_run_time () {
    local verbose=$1

    local start_serial="$(date +%s%N)"
    ./$SERIAL_NAME $TEST_DIR/*.in > ./$TEST_DIR-out/speed_$SERIAL_NAME.out
    #Get the time in milliseconds
    local duration_serial=$[ ($(date +%s%N) - ${start_serial}) / 1000000 ]

    local start="$(date +%s%N)"
    # use the default number of threads 
    ./$NAME 0 $TEST_DIR/*.in > ./$TEST_DIR-out/speed_$NAME.out
    #Get the time in milliseconds
    duration=$[ ($(date +%s%N) - ${start}) / 1000000 ]


    # Check that the pwzipv3 binary worked properly.
    returnval=$(diff ./$TEST_DIR-out/speed_$SERIAL_NAME.out ./$TEST_DIR-out/speed_$NAME.out)
    if (( $? != 0 )); then
      echo "parallized zip output does not match expected results. Please ensure your code passes all part 1 test cases."
      exit 1
    fi

    if (( $verbose == 1 )); then
      echo "$SERIAL_NAME runtime: ${duration_serial} milliseconds."
      echo "$NAME runtime: ${duration} milliseconds."
      print_result $duration $duration_serial
    fi

    total_duration=$(($total_duration+$duration))
    total_duration_serial=$(($total_duration_serial+$duration_serial))
}

print_result () {
  local duration=$1
  local duration_serial=$2

  if [ ${duration} -gt ${duration_serial} ];
  then
    echo "$NAME was slower than $SERIAL_NAME!"
    slowdown=`echo - | awk "{print ${duration} / ${duration_serial}}"`
    builtin echo -e "\e[33mThe slowdown is ${slowdown}X.\e[0m"
  else
    echo "$NAME was faster than $SERIAL_NAME!"
    speedup=`echo - | awk "{print ${duration_serial} / ${duration}}"`
    builtin echo -e "\e[32mThe speedup is ${speedup}X.\e[0m"
  fi
}

# usage: call when args not parsed, or when help needed
usage () {
    echo "usage: test_speed.sh [-h] [-v]"
    echo "  -h                help message"
    echo "  -v                verbose"
    echo "  -r n              repeat for n times; default 10"
    return 0
}

precheck () {
  # local NAME=$1
  # local SERIAL_NAME=$2

  WORKING_DIR=$(pwd)
  SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

  if [[ "$WORKING_DIR" != "$SCRIPT_DIR" ]]; then
    echo "Please rerun this script from: $SCRIPT_DIR"
    exit 1
  fi

  PARENT_DIR="$(dirname "$SCRIPT_DIR")"

  if ! [[ -f "$PARENT_DIR/$NAME.c" ]]; then
      echo "Error: Could not find $NAME.c in directory: $PARENT_DIR"
      exit 1
  fi

  if [[ -x "$NAME" ]]; then
      #echo "Removing old $NAME binary."
      rm "$NAME"
  fi

  if [[ -x "$NAME" ]]; then
      echo "Error: Could not remove old "$NAME" binary."
      exit 1
  fi

  if ! [[ -f "$PARENT_DIR/$SERIAL_NAME.c" ]]; then
      echo "Error: Could not find goatzip.c in directory: $PARENT_DIR"
      exit 1
  fi
}

setup () {
  # need a test directory; must be named "tests-out"
  if [[ ! -d $TEST_DIR-out ]]; then
      mkdir $TEST_DIR-out
  fi

  # compile with optimizatin turns on.
  gcc -Wall -Werror -pthread -O -o $NAME ../$NAME.c
  gcc -Wall -Werror -O -o $SERIAL_NAME ../$SERIAL_NAME.c

  ## Warmup the test -- otherwise the first run of measurement can be weird
  ./$SERIAL_NAME $TEST_DIR/*.in > ./$TEST_DIR-out/speed_$SERIAL_NAME.out
  ./$NAME 0 $TEST_DIR/*.in > ./$TEST_DIR-out/speed_$NAME.out
  ##
}


# main
TEST_DIR=$1
NAME=wpzip
SERIAL_NAME=wzip

verbose=0

repeat=5
total_duration=0
total_duration_serial=0


args=`getopt hvr: $*`
if [[ $? != 0 ]]; then
    usage; exit 1
fi

set -- $args
for i; do
    case "$i" in
    -h)
	usage
	exit 0
        shift;;
    -v)
        verbose=1
        shift;;
    -r)
        repeat=$2
	shift
	number='^[0-9]+$'
	if ! [[ $repeat =~ $number ]]; then
	    usage
	    echo "-t must be followed by a number" >&2; exit 1
	fi
        shift;;
    esac
done


precheck
setup


# https://www.cyberciti.biz/faq/unix-linux-iterate-over-a-variable-range-of-numbers-in-bash/
for (( i=1; i<=$repeat; i++ ))
# for i in {1..$repeat..1}
do
  if (( $verbose == 1 )); then
    echo "Running test: $i"
  fi
  measure_run_time $verbose
done

echo "---ready for the result---"
print_result $total_duration $total_duration_serial
