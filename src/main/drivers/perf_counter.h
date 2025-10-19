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
 * @file perf_counter.h
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief functions to measure performance of code excecution
 * @date 2025-07-11
 */

#pragma once

typedef struct perfCounts_s {
    uint32_t instructionCounter;
    uint32_t clockCounter;
} perfCounts_t;

extern perfCounts_t perfCounts;

void Perf_resetAndStartCounters();
void Perf_stopCounters();
