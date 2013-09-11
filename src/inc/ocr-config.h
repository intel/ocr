/**
 * @brief Configuration for the OCR runtime
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_CONFIG_H__
#define __OCR_CONFIG_H__

// TODO: This file will be rendered obsolete soon

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#ifdef OCR_ENABLE_STATISTICS
// For now have a central statistics aggregator filter
ocrStatsFilter_t   *GocrFilterAggregator;

// HUGE HUGE HACK: it seems that currently we can run code outside of EDTs
// which is a big problem since then we don't have a source "process" for Lamport's
// clock. This should *NEVER* be the case and there should be no non-EDT code
// except for the bootstrap. For now, and for expediency, this is a hack where I have
// a fake Lamport process for the initial non-EDT code
ocrStatsProcess_t GfakeProcess;

#endif

#endif /* __OCR_CONFIG_H__ */
