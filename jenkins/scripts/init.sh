#!/bin/bash

# Make sure the Jenkins system is fully accessible in the shared home
mkdir -p ${JJOB_SHARED_HOME}/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ss/jenkins
mkdir -p ${JJOB_SHARED_HOME}/ocr/jenkins

cp -r ${JJOB_PRIVATE_HOME}/jenkins/* ${JJOB_SHARED_HOME}/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ss/jenkins/* ${JJOB_SHARED_HOME}/ss/jenkins/
cp -r ${JJOB_PRIVATE_HOME}/ocr/jenkins/* ${JJOB_SHARED_HOME}/ocr/jenkins/

