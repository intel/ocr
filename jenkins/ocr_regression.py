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

jobtype_ocr_runtest = {
    'name': 'ocr-runtest',
    'isLocal': True,
    'run-cmd': '${JJOB_START_HOME}/ocr/jenkins/scripts/runtest.sh',
    'param-cmd': '${JJOB_START_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr',),
    'timeout': 120,
    'sandbox':('local', 'shareOK')
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

job_ocr_runtest_x86_pthread_fsim_1 = {
    'name': 'ocr-runtest-x86-pthread-fsim-1',
    'depends': ('ocr-buildtest-x86-pthread-fsim-1',),
    'jobtype': 'ocr-runtest',
    'run-args': 'x86-pthread-fsim ${JJOB_START_HOME}/ocr/examples/fsim_x86_test golden.txt ./helloworld.exe -ocr:cfg ${JJOB_START_HOME}/ocr/install/x86-pthread-fsim/config/default.cfg',
    'sandbox': ('inherit0',)
}
