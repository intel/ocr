#!/usr/bin/env python

jobtype_ocr_regression = {
    'name': 'ocr-regression',
    'isLocal': True,
    'run-cmd': '${JJOB_START_HOME}/ocr/jenkins/scripts/regression.sh',
    'param-cmd': '${JJOB_START_HOME}/ocr/jenkins/scripts/regression.sh _params',
    'keywords': ('ocr',),
    'timeout': 240,
    'sandbox': ('local', 'shareOK')
}

job_ocr_regression_x86_pthread_x86 = {
    'name': 'ocr-regression-x86-pthread-x86',
    'depends': ('ocr-build-x86-pthread-x86',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-x86',
    'sandbox': ('inherit0',)
}


    
