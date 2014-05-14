#!/bin/bash
make run 2> test.txt
date
grep "Create" ./test.txt | wc -l
date
grep "Execute" ./test | wc -l
date
grep "Destroy" ./test | wc -l
