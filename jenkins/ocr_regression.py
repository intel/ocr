#!/usr/bin/env python

import os

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
    'run-args': 'x86-pthread-x86 jenkins-hc-8w-regularDB.cfg regularDB',
    'sandbox': ('inherit0',)
}

job_ocr_regression_x86_pthread_x86_lockableDB = {
    'name': 'ocr-regression-x86-pthread-x86-lockableDB',
    'depends': ('ocr-build-x86-pthread-x86',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-x86 jenkins-hc-8w-lockableDB.cfg lockableDB',
    'sandbox': ('inherit0',)
}

job_ocr_regression_x86_pthread_tg_regularDB = {
    'name': 'ocr-regression-x86-pthread-tg-regularDB',
    'depends': ('ocr-build-x86-pthread-tg',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-tg jenkins-1block-regularDB.cfg regularDB',
    'sandbox': ('inherit0',)
}

job_ocr_regression_x86_pthread_tg_lockableDB = {
    'name': 'ocr-regression-x86-pthread-tg-lockableDB',
    'depends': ('ocr-build-x86-pthread-tg',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-tg jenkins-1block-lockableDB.cfg lockableDB',
    'sandbox': ('inherit0',)
}

#TODO: not sure how to not hardcode MPI_ROOT here
job_ocr_regression_x86_pthread_mpi_lockableDB = {
    'name': 'ocr-regression-x86-pthread-mpi-lockableDB',
    'depends': ('ocr-build-x86-pthread-mpi',),
    'jobtype': 'ocr-regression',
    'run-args': 'x86-pthread-mpi jenkins-hc-dist-mpi-clone-8w-lockableDB.cfg lockableDB',
    'sandbox': ('inherit0',),
    'env-vars': {'MPI_ROOT': '/opt/intel/tools/impi/5.0.1.035',
                 'PATH': '${MPI_ROOT}/bin64:'+os.environ['PATH'],
                 'LD_LIBRARY_PATH': '${MPI_ROOT}/lib64',}
}
