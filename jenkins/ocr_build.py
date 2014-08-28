#!/usr/bin/env python

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

job_ocr_build_x86_pthread_fsim = {
    'name': 'ocr-build-x86-pthread-fsim',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-pthread-fsim',
    'sandbox': ('inherit0',)
}

job_ocr_build_x86_builder_fsim_ce = {
    'name': 'ocr-build-x86-builder-fsim-ce',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-builder-fsim-ce',
    'sandbox': ('inherit0',)
}

job_ocr_build_x86_builder_fsim_xe = {
    'name': 'ocr-build-x86-builder-fsim-xe',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'x86-builder-fsim-xe',
    'sandbox': ('inherit0',)
}

job_ocr_build_fsim_null_fsim_ce = {
    'name': 'ocr-build-fsim-null-fsim-ce',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'fsim-null-fsim-ce',
    'sandbox': ('inherit0',)
}

job_ocr_build_fsim_null_fsim_xe = {
    'name': 'ocr-build-fsim-null-fsim-xe',
    'depends': ('ocr-init-job',),
    'jobtype': 'ocr-build',
    'run-args': 'fsim-null-fsim-xe',
    'sandbox': ('inherit0',)
}
