******************
** Requirements **
******************

Build requirements:

    - make

******************
** Installation **
******************

** Quick installation of x86-pthread-x86 target

1) Go to the build directory

2) Run the command: ARCH=x86-pthread-x86 make install
    
3) Go back to the main OCR directory and Set environment variable 

    export OCR_INSTALL=${PWD}/install/x86-pthread-x86/
    export LD_LIBRARY_PATH=${OCR_INSTALL}/lib:${LD_LIBRARY_PATH}

3) Run helloworld

    cd examples/hello
    make


** Installation information

The build system allows the building of OCR for multiple targets.
Each target has its own subfolder in the 'build' folder and will
install its finished products (libraries, headers, etc.) in its
own subfolder in the 'install' folder. 

Each target usually has the following make targets (not all
are supported on all targets):
    - shared:         Builds a shared library object for OCR
    - static:         Builds a static library object for OCR
    - exec:           Builds a policy domain builder (executable) for OCR
    - debug-shared:   Builds a shared library object with debugging symbols
    - debug-static:   Builds a static library object with debugging symbols
    - debug-exec:     Builds a policy domain builder with debugging symbols
    - install:        Installs libraries, etc. in install/${ARCH}
    - clean:          Removes build products (.o etc.)
    - uninstall:      Removes installed libraries, etc.
