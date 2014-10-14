#!/bin/sh

egrep -w 'EDT\(INFO\)|EVT\(INFO\)' log.txt | egrep -w 'FctName' > filtered.txt

