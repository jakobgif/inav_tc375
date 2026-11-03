/*
 * This file is part of INAV.
 *
 * INAV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * INAV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with INAV.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file perf_counter.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief functions to measure performance of code excecution
 * @date 2025-07-11
 */

#include "platform.h"

#include "build/build_config.h"
#include "drivers/perf_counter.h"

#ifdef USE_PERFCOUNTER

FASTRAM perfCounts_t perfCounts;

#if defined(TC375)

/**
 * Note: Aurix performance counters only work when the On-Chip Debug System is running. 
 * Therefore the performance counters only work withing an active debug session. 
 * However, the code can still be optimized as normal. The debug session should have no impact on the system performance.
 */

inline __attribute__ ((always_inline)) void Perf_resetAndStartCounters(){
    IfxCpu_resetAndStartCounters(IfxCpu_CounterMode_normal);
}

STATIC_FASTRAM IfxCpu_Perf perf;

inline __attribute__ ((always_inline)) void Perf_stopCounters(){
    perf = IfxCpu_stopCounters();
    perfCounts.instructionCounter = perf.instruction.counter;
    perfCounts.clockCounter = perf.clock.counter;
}

#endif // TC375

#else

void Perf_resetAndStartCounters(){
    return;
}

void Perf_stopCounters(){
    return;
}

#endif // USE_PERFCOUNTER
