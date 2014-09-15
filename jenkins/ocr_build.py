#!/usr/bin/env python

import os

jobtype_ocr_init = {
    'name': 'ocr-init',
    'isLocal': True,
    'run-cmd': '${JJOB_PRIVATE_HOME}/ocr/jenkins/scripts/init.sh',
    'param-cmd': '${JJOB_PRIVATE_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 20,
    'sandbox': ('local', 'shared', 'emptyShared', 'shareOK')
}

# Note that we could do away with the copy entirely and just
# copy the build directory but keeping for now
jobtype_ocr_build = {
    'name': 'ocr-build',
    'isLocal': True,
    'run-cmd': '${JJOB_PRIVATE_HOME}/ocr/jenkins/scripts/build.sh',
    'param-cmd': '${JJOB_PRIVATE_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 120,
    'sandbox': ('local', 'shared', 'shareOK'),
    'env-vars': {'TG_INSTALL': '${JJOB_ENVDIR}',
                 'TG_SRC': '${JJOB_PRIVATE_HOME}/ss',
                 'OCR_SRC': '${JJOB_PRIVATE_HOME}/ocr',
                 'OCR_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/build',
                 'OCR_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/install'}
}

# Specific jobs

job_ocr_init = {
    'name': 'ocr-init-job',
    'depends': ('ss-build-check-env',),
    'jobtype': 'ocr-init',
    'run-args': ''
}

job_ocr_build_x86_pthread_x86 = {
    'name': 'ocr-build-x86-pthread-x86',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-x86',
    'sandbox': ('inherit0',)
}

#TODO: not sure how to not hardcode MPI_ROOT here
job_ocr_build_x86_pthread_mpi = {
    'name': 'ocr-build-x86-pthread-mpi',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-mpi',
    'sandbox': ('inherit0',),
    'env-vars': {'MPI_ROOT': '/opt/intel/tools/impi/5.0.1.035',
                 'PATH': '${MPI_ROOT}/bin64:'+os.environ['PATH'],}
}

job_ocr_build_x86_pthread_tg = {
    'name': 'ocr-build-x86-pthread-tg',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-tg',
    'sandbox': ('inherit0',)
}

job_ocr_build_x86_builder_tg_ce = {
    'name': 'ocr-build-x86-builder-tg-ce',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-builder-tg-ce',
    'sandbox': ('inherit0',)
}

job_ocr_build_x86_builder_tg_xe = {
    'name': 'ocr-build-x86-builder-tg-xe',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-builder-tg-xe',
    'sandbox': ('inherit0',)
}

job_ocr_build_tg_null_tg_ce = {
    'name': 'ocr-build-tg-null-tg-ce',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'tg-null-tg-ce',
    'sandbox': ('inherit0',)
}

job_ocr_build_tg_null_tg_xe = {
    'name': 'ocr-build-tg-null-tg-xe',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'tg-null-tg-xe',
    'sandbox': ('inherit0',)
}
