#!/bin/bash 

fail() {

	echo
	echo " FATAL ERROR  -  pushing terminated "
	echo
	exit $1
}

if [ $# -ne 2 ]
then 
	echo "Usage:"
	echo "  $0 <version>"
	exit 0
fi

ver=$2

git push || fail $?
git push origin $ver || fail $?

