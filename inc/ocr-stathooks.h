/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS
#ifdef OCR_ENABLE_PROFILING_STATISTICS

#ifndef __OCR_STATHOOKS_H__
#define __OCR_STATHOOKS_H__

#ifdef __cplusplus
extern "C" {
#endif

extern __thread uint64_t _threadInstructionCount;
extern __thread uint8_t  _threadInstrumentOn;

extern void PROFILER_ocrStatsLoadCallback(void* addr, u64 size, u64 instrCount);
extern void PROFILER_ocrStatsStoreCallback(void* addr, u64 size, u64 instrCount);

#ifdef __cplusplus
}
#endif

#endif /* __OCR_STATHOOKS_H__ */

#endif /* OCR_ENABLE_PROFILING_STATISTICS */
#endif /* OCR_ENABLE_STATISTICS */
