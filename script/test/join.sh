#!/bin/sh

sed '/^#/d' 1 | sort > 1.sorted
sed '/^#/d' 2 | sort > 2.sorted

join 1.sorted 2.sorted > result
rm *.sorted

