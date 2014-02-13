#!/bin/bash

cd ${JJOB_START_HOME}/ocr/build/$1
make shared static install

exit $?
