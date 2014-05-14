/***************************************************************************
=============================================================================
SCF-NWCHEM for OCR

Jaime Arteaga, jaime@udel.edu, August 2013

Based on the code found on https://www.xstackwiki.com/index.php/DynAX.

=============================================================================

Copyright (c) 2013, University of Delaware
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef SCF_H
#define SCF_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef USE_MKL
#include <mkl_cblas.h>
#elif defined BLAS
#include <cblas.h>
#endif

#include "cscc.h"
#include "integ.h"
#include "input.h"
#include "output.h"
#include "diagonalize.h"
#include "g.h"
#include "twoel.h"

extern void mkpre(void);
extern void setfm(void);
extern void ininrm(void);
extern void setarrays(void);
extern void setvectors(void);
extern void denges(void);
extern void makeob(void);
extern double s(int i, int j);
extern double makesz();
extern double oneel(double schwmax);
extern double contract_matrices(double * a, double * b);
extern double h(int i,int j);
extern void closearrays(void);
extern void makden(void);
extern void damp(double fac);

#endif
