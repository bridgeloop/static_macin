#!/bin/sh

d=$(realpath $(dirname $0))
cd "$d"
clean() {
	cd "$d"
	rm -rf build
	exit $1
}

mkdir build
cd build
gcc -c -O2 ../src/*.c $@
if [ $? -ne 0 ]; then
	clean 1
fi
ld -r -o ../output/static_macin.o *.o
if [ $? -ne 0 ]; then
	clean 1
fi

cd ..
rm -rf build
exit 0