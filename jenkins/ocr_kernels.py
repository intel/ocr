#!/usr/bin/env python

# This relies on stuff (OCR) being built in the shared
# home but does not need any separate checkout as it
# runs completely out of the source directory
jobtype_ocr_build_kernel = {
    'name': 'ocr-build-kernel',
    'isLocal': True,
    'run-cmd': '${JJOB_SHARED_HOME}/ocr/jenkins/scripts/kernel-build.sh',
    'param-cmd': '${JJOB_SHARED_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 300,
    'sandbox': ('local', 'shared', 'shareOK'),
    'env-vars': { 'RMD_INSTALL': '${JJOB_ENVDIR}',
                  'RMD_SRC': '${JJOB_INITDIR}/ss',
                  'OCR_SRC': '${JJOB_PRIVATE_HOME}/ocr',
                  'OCR_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/install',
                  'OCR_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/build'}
}

jobtype_ocr_run_kernel_local = {
    'name': 'ocr-run-kernel-local',
    'isLocal': True,
    'run-cmd': '${JJOB_SHARED_HOME}/ocr/jenkins/scripts/kernel-run-local.sh',
    'param-cmd': '${JJOB_SHARED_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 300,
    'sandbox': ('shared', 'shareOK'),
    'env-vars': { 'RMD_INSTALL': '${JJOB_ENVDIR}',
                  'OCR_BUILD_ROOT': '${JJOB_PARENT_PRIVATE_HOME_0}/ocr/build',
                  'OCR_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/install'}
}

# FIXME: Make this use the make run...
jobtype_ocr_run_kernel_remote = {
    'name': 'ocr-run-kernel-remote',
    'isLocal': False,
    'run-cmd': '${JJOB_SHARED_HOME}/ocr/jenkins/scripts/kernel-run-remote.sh',
    'param-cmd': '${JJOB_SHARED_HOME}/ss/jenkins/scripts/fsim-param-cmd.sh',
    'epilogue-cmd': '${JJOB_ENVDIR}/bin/fsim-scripts/fsim-epilogue.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 300,
    'sandbox': ('shared', 'shareOK'),
    'env-vars': { 'RMD_INSTALL': '${JJOB_ENVDIR}',
                  'OCR_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/install'}
}

jobtype_ocr_verify_kernel_local = {
    'name': 'ocr-verify-kernel-local',
    'isLocal': True,
    'run-cmd': '${JJOB_SHARED_HOME}/ocr/jenkins/scripts/ocr-verify-local.sh',
    'param-cmd': '${JJOB_SHARED_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 300,
    'sandbox': ('shared', 'shareOK'),
}

jobtype_ocr_verify_kernel_remote = {
    'name': 'ocr-verify-kernel-remote',
    'isLocal': True,
    'run-cmd': '${JJOB_ENVDIR}/jenkins/scripts/fsim-verify.sh',
    'param-cmd': '${JJOB_SHARED_HOME}/jenkins/scripts/empty-cmd.sh',
    'keywords': ('ocr', 'percommit'),
    'timeout': 300,
    'sandbox': ('shared', 'shareOK'),
}

# Specific jobs

# Fibonacci

job_ocr_build_kernel_fib_x86 = {
    'name': 'ocr-build-kernel-fib-x86',
    'depends': ('ocr-build-x86-pthread-x86',),
    'jobtype': 'ocr-build-kernel',
    'run-args': 'fib x86-pthread-x86',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fib',
                  'WORKLOAD_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/ocr-apps/fib/build',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install'}
}

job_ocr_run_kernel_fib_x86 = {
    'name': 'ocr-run-kernel-fib-x86',
    'depends': ('ocr-build-kernel-fib-x86',),
    'jobtype': 'ocr-run-kernel-local',
    'run-args': 'fib x86-pthread-x86',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fib',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install' }
}

job_ocr_verify_kernel_fib_x86 = {
    'name': 'ocr-verify-kernel-fib-x86',
    'depends': ('ocr-run-kernel-fib-x86',),
    'jobtype': 'ocr-verify-kernel-local',
    'run-args': '-w -c 1',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_EXEC': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install/x86-pthread-x86'}
}

job_ocr_build_kernel_fib_tgemul = {
    'name': 'ocr-build-kernel-fib-tgemul',
    'depends': ('ocr-build-x86-pthread-fsim',),
    'jobtype': 'ocr-build-kernel',
    'run-args': 'fib x86-pthread-fsim',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fib',
                  'WORKLOAD_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/ocr-apps/fib/build',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install'}
}

job_ocr_run_kernel_fib_tgemul = {
    'name': 'ocr-run-kernel-fib-tgemul',
    'depends': ('ocr-build-kernel-fib-tgemul',),
    'jobtype': 'ocr-run-kernel-local',
    'run-args': 'fib x86-pthread-fsim',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fib',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install'}
}

job_ocr_verify_kernel_fib_tgemul = {
    'name': 'ocr-verify-kernel-fib-tgemul',
    'depends': ('ocr-run-kernel-fib-tgemul',),
    'jobtype': 'ocr-verify-kernel-local',
    'run-args': '-w -c 1',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_EXEC': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install/x86-pthread-fsim'}
}

job_ocr_build_kernel_fib_fsim = {
    'name': 'ocr-build-kernel-fib-fsim',
    'depends': ('ocr-build-x86-builder-fsim-ce', 'ocr-build-x86-builder-fsim-xe',
                'ocr-build-fsim-null-fsim-ce', 'ocr-build-fsim-null-fsim-xe'),
    'jobtype': 'ocr-build-kernel',
    'run-args': 'fib whole-fsim',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fib',
                  'WORKLOAD_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/ocr-apps/fib/build',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install'}
}

job_ocr_run_kernel_fib_fsim = {
    'name': 'ocr-run-kernel-fib-fsim',
    'depends': ('ocr-build-kernel-fib-fsim',),
    'jobtype': 'ocr-run-kernel-remote',
    'run-args': 'fib whole-fsim',
    'param-args': '-c ${WORKLOAD_INSTALL_ROOT}/whole-fsim/config.cfg',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install'}
}

job_ocr_verify_kernel_fib_fsim = {
    'name': 'ocr-verify-kernel-fib-fsim',
    'depends': ('ocr-run-kernel-fib-fsim',),
    'jobtype': 'ocr-verify-kernel-remote',
    'run-args': '-w -c 1',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_EXEC': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fib/install/whole-fsim'}
}

# FFT

job_ocr_build_kernel_fft_x86 = {
    'name': 'ocr-build-kernel-fft-x86',
    'depends': ('ocr-build-x86-pthread-x86',),
    'jobtype': 'ocr-build-kernel',
    'run-args': 'fft x86-pthread-x86',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fft',
                  'WORKLOAD_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/ocr-apps/fft/build',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install'}
}

job_ocr_run_kernel_fft_x86 = {
    'name': 'ocr-run-kernel-fft-x86',
    'depends': ('ocr-build-kernel-fft-x86',),
    'jobtype': 'ocr-run-kernel-local',
    'run-args': 'fft x86-pthread-x86',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fft',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install' }
}

job_ocr_verify_kernel_fft_x86 = {
    'name': 'ocr-verify-kernel-fft-x86',
    'depends': ('ocr-run-kernel-fft-x86',),
    'jobtype': 'ocr-verify-kernel-local',
    'run-args': '-w -c 1',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_EXEC': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install/x86-pthread-x86'}
}

job_ocr_build_kernel_fft_tgemul = {
    'name': 'ocr-build-kernel-fft-tgemul',
    'depends': ('ocr-build-x86-pthread-fsim',),
    'jobtype': 'ocr-build-kernel',
    'run-args': 'fft x86-pthread-fsim',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fft',
                  'WORKLOAD_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/ocr-apps/fft/build',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install'}
}

job_ocr_run_kernel_fft_tgemul = {
    'name': 'ocr-run-kernel-fft-tgemul',
    'depends': ('ocr-build-kernel-fft-tgemul',),
    'jobtype': 'ocr-run-kernel-local',
    'run-args': 'fft x86-pthread-fsim',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fft',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install'}
}

job_ocr_verify_kernel_fft_tgemul = {
    'name': 'ocr-verify-kernel-fft-tgemul',
    'depends': ('ocr-run-kernel-fft-tgemul',),
    'jobtype': 'ocr-verify-kernel-local',
    'run-args': '-w -c 1',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_EXEC': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install/x86-pthread-fsim'}
}

job_ocr_build_kernel_fft_fsim = {
    'name': 'ocr-build-kernel-fft-fsim',
    'depends': ('ocr-build-x86-builder-fsim-ce', 'ocr-build-x86-builder-fsim-xe',
                'ocr-build-fsim-null-fsim-ce', 'ocr-build-fsim-null-fsim-xe'),
    'jobtype': 'ocr-build-kernel',
    'run-args': 'fft whole-fsim',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_SRC': '${JJOB_INITDIR}/ocr/ocr-apps/fft',
                  'WORKLOAD_BUILD_ROOT': '${JJOB_PRIVATE_HOME}/ocr/ocr-apps/fft/build',
                  'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install'}
}

job_ocr_run_kernel_fft_fsim = {
    'name': 'ocr-run-kernel-fft-fsim',
    'depends': ('ocr-build-kernel-fft-fsim',),
    'jobtype': 'ocr-run-kernel-remote',
    'run-args': 'fft whole-fsim',
    'param-args': '-c ${WORKLOAD_INSTALL_ROOT}/whole-fsim/config.cfg',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_INSTALL_ROOT': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install'}
}

job_ocr_verify_kernel_fft_fsim = {
    'name': 'ocr-verify-kernel-fft-fsim',
    'depends': ('ocr-run-kernel-fft-fsim',),
    'jobtype': 'ocr-verify-kernel-remote',
    'run-args': '-w -c 1',
    'sandbox': ('inherit0',),
    'env-vars': { 'WORKLOAD_EXEC': '${JJOB_SHARED_HOME}/ocr/ocr-apps/fft/install/whole-fsim'}
}
