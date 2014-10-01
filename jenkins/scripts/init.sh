#!/bin/bash

# We start out in JJOB_PRIVATE_HOME

# Make sure the Jenkins system is fully accessible in the shared home
# We also copy any apps/ makefiles and datasets
mkdir -p ${JJOB_SHARED_HOME}/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ss/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ocr/jenkins
mkdir -p ${JJOB_SHARED_HOME}/apps

cp -r ${JJOB_PRIVATE_HOME}/jenkins/* ${JJOB_SHARED_HOME}/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ss/jenkins/* ${JJOB_SHARED_HOME}/ss/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ocr/jenkins/* ${JJOB_SHARED_HOME}/ocr/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/apps/makefiles ${JJOB_SHARED_HOME}/apps/
cd apps/
find . -type d -name "datasets" -print0 | xargs -0 cp -a --parents -t ${JJOB_SHARED_HOME}/apps/

