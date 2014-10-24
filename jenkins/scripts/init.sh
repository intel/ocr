#!/bin/bash

# We start out in JJOB_PRIVATE_HOME

# Make sure the Jenkins system is fully accessible in the shared home
# We also copy any apps/ makefiles and datasets
mkdir -p ${JJOB_SHARED_HOME}/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ss/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ocr/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ocr/ocr-apps
mkdir -p ${JJOB_SHARED_HOME}/apps

cp -r ${JJOB_PRIVATE_HOME}/jenkins/* ${JJOB_SHARED_HOME}/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ss/jenkins/* ${JJOB_SHARED_HOME}/ss/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ocr/jenkins/* ${JJOB_SHARED_HOME}/ocr/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ocr/ocr-apps/* ${JJOB_SHARED_HOME}/ocr/ocr-apps/
rsync -av -r ${JJOB_PRIVATE_HOME}/apps/ ${JJOB_SHARED_HOME}/apps/ --exclude libs --exclude makefiles

mkdir -p ${JJOB_SHARED_HOME}/apps/libs/tg/lib
mkdir -p ${JJOB_SHARED_HOME}/apps/libs/tg/include
mkdir -p ${JJOB_SHARED_HOME}/apps/libs/x86/lib
mkdir -p ${JJOB_SHARED_HOME}/apps/libs/x86/include
ln -s ${JJOB_SHARED_HOME}/ocr/ocr-apps/makefiles ${JJOB_SHARED_HOME}/apps/makefiles
