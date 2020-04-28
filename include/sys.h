//
// Created by JiangGuoqing on 2018/10/10.
//

#ifndef DG_FEATURE_H
#define DG_FEATURE_H


#define MONITOR 0
#define LOGFULL VLOG(22)
//#define LOGFULL LOG(INFO)

#define VEGA_UNUSED(x) (void)x

#if HISI
#define USE_DGJSON 1

#if HI3559A
#define NNIE 2      // NNIE core count
#define IVE 1      // NNIE core count
#define MALI 1      // MALI core count
#elif (HI3516 || HI3519)
#define NNIE 1      // NNIE core count
#define IVE 1      // IVE core count
#define MALI 0      // MALI core count
#endif

#define PERF_TIME 0
#if PERF_TIME
#include <sys/time.h>
inline long getCostMs(struct timeval start, struct timeval end) {
    return 1000000 * (end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec;
}
inline long toUs(struct timeval timer) {
    return 1000000 * timer.tv_sec + timer.tv_usec;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#elif HIAI
#define USE_DGJSON 1


////////////////////////////////////////////////////////////////////////////////////////////////////
#elif CUDA
#if NEWCUDA
#define USE_DGJSON 1
#else
#define USE_DGJSON 0
#endif
#endif


#endif //DG_FEATURE_H
