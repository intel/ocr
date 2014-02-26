#!/usr/bin/env python

jobtype_ocr_build = {
    'name': 'ocr-build',
    'isLocal': True,
    'run-cmd': '${JJOB_START_HOME}/ocr/jenkins/scripts/build.sh',
    'param-cmd': '${JJOB_START_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr',),
    'timeout': 120,
    'sandbox': ('local', 'shareOK')
}

jobtype_ocr_buildtest = {
    'name': 'ocr-buildtest',
    'isLocal': True,
    'run-cmd': '${JJOB_START_HOME}/ocr/jenkins/scripts/buildtest.sh',
    'param-cmd': '${JJOB_START_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr',),
    'timeout': 120,
    'sandbox':('local', 'shareOK')
}

# Specific jobs

job_ocr_build_x86_pthread_x86 = {
    'name': 'ocr-build-x86-pthread-x86',
    'depends': (),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-x86'
}

job_ocr_build_x86_pthread_fsim = {
    'name': 'ocr-build-x86-pthread-fsim',
    'depends': (),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-fsim'
}

job_ocr_buildtest_x86_pthread_fsim_1 = {
    'name': 'ocr-buildtest-x86-pthread-fsim-1',
    'depends': ('ocr-build-x86-pthread-fsim',),
    'jobtype': 'ocr-buildtest',
    'run-args': 'x86-pthread-fsim ${JJOB_START_HOME}/ocr/examples/fsim_x86_test make compile',
    'sandbox': ('inherit0',)
}



    
