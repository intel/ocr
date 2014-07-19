#!/usr/bin/env python

# Make a copy of the local home because the regressions run
# in place for now
jobtype_ocr_regression = {
    'name': 'ocr-regression',
    'isLocal': True,
    'run-cmd': '${JJOB_PRIVATE_HOME}/ocr/jenkins/scripts/regression.sh',
    'param-cmd': '${JJOB_PRIVATE_HOME}/ocr/jenkins/scripts/regression.sh _params',
    'keywords': ('ocr', 'percommit'),
    'timeout': 240,
    'sandbox': ('local', 'shared', 'copyLocal', 'shareOK')
}

# Specific jobs

job_ocr_regression_x86_pthread_x86 = {
    'name': 'ocr-regression-x86-pthread-x86',
    'depends': ('ocr-build-x86-pthread-x86',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-x86',
    'sandbox': ('inherit0',)
}

job_ocr_regression_x86_pthread_fsim = {
    'name': 'ocr-regression-x86-pthread-fsim',
    'depends': ('ocr-build-x86-pthread-fsim',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-fsim',
    'sandbox': ('inherit0',)
}



