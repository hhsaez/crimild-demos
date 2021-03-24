#!/bin/bash

basepath="examples"

ignore=("/CMakeFiles" "/Makefile" "/CMakeLists.txt")

examples=$(find "${basepath}" \
	-maxdepth 1 \
	-not \( -path "*${ignore[0]}" $(printf -- '-o -path "*%s" ' "${ignore[@]:1}") \) \
	-not -path "${basepath}" \
	-type d | sort -t '\0' -n)

for d in ${examples}
do
	example_name="${d##*/}"
	echo "Running ${example_name}"
	${basepath}/${example_name}/${example_name} &> /dev/null
done

