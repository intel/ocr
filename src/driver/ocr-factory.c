/* Copyright (c) 2012, Rice University

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

*/

#include <assert.h>

#include "ocr-factory.h"

//TODO we are deprecating this file

//TODO don't like that: I think we should have some kind of a plugin
// mechanism where an implementation can associate its string to a function pointer.
// i.e HC_WORKPOOL -> hc_workpile_constructor
extern ocr_workpile_t * hc_workpile_constructor();

/******************************************************/
/* WORKPILE */
/******************************************************/

ocr_workpile_t * newWorkpile(ocr_workpile_kind workpileType) {
    switch(workpileType) {
    case OCR_DEQUE:
        return hc_workpile_constructor();
    }
    assert(false && "Unrecognized workpile kind");
    return NULL;
}
