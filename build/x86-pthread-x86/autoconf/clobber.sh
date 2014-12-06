#!/bin/sh

#
# Cleanup the project
#

SRCDIR=../../../src

rm -Rf compileTree

rm -Rf config/* autom4te.cache aclocal.m4 configure COPYING depcomp config.log config.status libtool

for file in `find ${SRCDIR} -name Makefile.in`; do
    rm -Rf $file
done

for file in `find ${SRCDIR} -name Makefile`; do
    rm -Rf $file
done

for file in `find ${SRCDIR} -regex '.*\.o$'`; do
    rm -Rf $file
done

for file in `find ${SRCDIR} -regex '.*\.lo$'`; do
    rm -Rf $file
done

for file in `find ${SRCDIR} -regex '.*\.la$'`; do
    rm -Rf $file
done

for file in `find ${SRCDIR} -regex '.*deps$'`; do
    rm -Rf $file
done

for file in `find ${SRCDIR} -regex '.*libs$'`; do
    rm -Rf $file
done

rm -f Makefile Makefile.in
