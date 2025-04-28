#! /bin/bash

# usage: call when args not parsed, or when help needed
usage () {
    echo "usage: test_part2.sh [-h] [-v] [-t test] [-c] [-s] [-d testdir]"
    echo "  -h                help message"
    echo "  -v                verbose"
    echo "  -t n              run only test n; n =1 or 2"
    return 0
}

specific=""

# note, -v and -r are supported by test_speed.sh script
args=`getopt ht:vr: $*`
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
    -t)
        specific=$2
	shift
	number='^[0-9]+$'
	if ! [[ $specific =~ $number ]]; then
	    usage
	    echo "-t must be followed by a number" >&2; exit 1
	fi
        shift;;
    esac
done

# run just one test.
if [[ $specific == 1 ]]; then
    echo "Test 1: A bunch of smaller files"
    ./test_speed.sh tests-$specific $*
    exit 0
fi

if [[ $specific == 2 ]]; then
    echo "Test 2: A mix of smaller and larger files"
    ./test_speed.sh tests-$specific $*
    exit 0
fi

if [[ $specific == 3 ]]; then
    echo "Test 3: A large file!"
    ./test_speed.sh tests-$specific $*
    exit 0
fi


# run all
if [[ $specific == "" ]]; then
    echo "Test 1: A bunch of smaller files"
    ./test_speed.sh tests-1 $*

    echo "Test 2: A mix of smaller and larger files"
    ./test_speed.sh tests-2 $*
    

    echo "Test 3: A large file!"
    ./test_speed.sh tests-3 $*

    exit 0
else
    builtin echo -e "\e[31mWe only have three tests for this part...\e[0m"
    usage;
    exit 1
fi
#
