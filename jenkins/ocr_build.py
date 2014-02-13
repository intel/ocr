#!/usr/bin/env python

jobtype_ocr_build = {
    'name': 'ocr-build',
    'isLocal': True,
    'run-cmd': '${JJOB_START_HOME}/ocr/jenkins/scripts/build.sh',
    'param-cmd': '${JJOB_START_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr',),
    'timeout': 120,
    'sandbox': ('local',)
}

job_ocr_build_x86_pthread_x86 = {
    'name': 'ocr-build-x86-pthread-x86',
    'depends': (),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-x86'
}


    
