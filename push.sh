#!/bin/bash 

fail() {

	echo
	echo " FATAL ERROR  -  pushing terminated "
	echo
	exit $1
}

if [ $# -ne 1 ]
then 
	echo "Usage:"
	echo "  $0 <version>"
	exit 0
fi

ver=$2

git push origin HEAD || fail $?
git push origin $ver || fail $?

