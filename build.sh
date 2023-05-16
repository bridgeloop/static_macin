#!/bin/sh

d=$(realpath $(dirname $0))
cd "$d"

m=inc/mach-o
check() {
	echo "59c2f268af2f719462d9450820a8dfa6bccb9b19b48bce96c656d0bdeefa8faf $m/fat.h
5a99c726b0092004538bf95f7df108d0a6268c70c581ff7cfdcafc44c6de57a3 $m/loader.h
9e1a328c1a9632fae48d4d95b09d377f5007374ddb8addad97e0841052304c96 $m/nlist.h" |
	sha256sum --check --status

	return $?
}
clean() {
	cd "$d"
	rm -rf build
	exit $1
}

if ! check; then
	base="https://opensource.apple.com/source/cctools/cctools-921/include/mach-o"
	curl -s $base/loader.h -o $m/loader.h -: $base/fat.h -o $m/fat.h -: $base/nlist.h -o $m/nlist.h
	if ! check; then
		rm -f $m/*
		clean 1
	fi
fi

if [ $? -ne 0 ]; then
	clean 1
fi

mkdir build
cd build
gcc -c -O2 ../src/*.c -I../inc $@
ld -r -o ../output/static_macin.o *.o

clean 0
