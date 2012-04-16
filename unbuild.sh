#!/bin/bash 

fail() {

	echo
	echo " FATAL ERROR  - unbuildin terminated "
	echo
	exit $1
}

if [ $# -ne 1 ]
then 
	echo "Usage:"
	echo "  $0 <version>"
	exit 0
fi

ver=$1

git reset $ver~1 || fail $?
git checkout debian/changelog || fail $?
git tag -d $ver || fail $?

