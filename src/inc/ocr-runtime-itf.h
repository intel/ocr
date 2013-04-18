/* Copyright (c) 2013, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Rice University
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

#ifndef OCR_RUNTIME_ITF_H_
#define OCR_RUNTIME_ITF_H_

#include "ocr-types.h"
#include "ocr-guid.h"

/**
 * @file OCR Runtime interface: Defines additional API for runtime implementers.
 */

/**
 *  @brief Get @ offset in the currently running edt's local storage
 *  \return NULL_GUID if there's no ELS support.
 *  \attention Must be call from within an edt code.
 **/
ocrGuid_t ocrElsGet(u8 offset);

/**
 *  @brief Set data @ offset in the currently running edt's local storage
 *  \remark no-op if there's no ELS support
 **/
void ocrElsSet(u8 offset, ocrGuid_t data);

/**
 *  @brief Get the currently executing edt.
 *  \return NULL_GUID if there's no edt running.
 **/
ocrGuid_t getCurrentEdt();

/**
 * @brief Executes an edt on top of the current edt stack.
 * Useful for blocking runtimes built on top of OCR.
 */
void ocrRtBlockedHelp();

#endif /* OCR_RUNTIME_ITF_H_ */
