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

// Deliberately NOT wrapped in #ifdef USE_FIU: this is pure configuration data
// (an enum + several byte arrays), with no FIU-specific function calls, so
// the PG can stay unconditionally compiled and its settings.yaml group can
// skip `condition: USE_FIU`. That avoids a documented Debug-build trap (see
// guidelines_fiu.md "Fault Mitigation Plan" -- a `condition: USE_FIU`
// settings group makes its SETTING_..._DEFAULT macros disappear in a Debug
// build, where USE_FIU is not defined, while consuming code that references
// them unconditionally would then fail to compile).

#include "config/parameter_group.h"
#include "config/parameter_group_ids.h"

#include "fiu/fiu_config.h"

PG_REGISTER_WITH_RESET_TEMPLATE(fiuSlotConfig_t, fiuSlotConfig, PG_FIU_SLOT_CONFIG, 0);

// All slots default to FIU_SLOT_NONE (0) and every per-motor toggle defaults
// to off (0) -- the designated initializer below zero-fills every field not
// explicitly listed, so the motorM0..motorM5 arrays don't need to be spelled
// out here. i2cMaxRate/spiMaxRate only matter for *_ANOMALY (*_STUCK is
// always a fixed 100%, see fiu_config.h) -- defaults are a starting point
// for the knob-scaled ceiling, retune freely via CLI.
PG_RESET_TEMPLATE(fiuSlotConfig_t, fiuSlotConfig,
    .slot = { FIU_SLOT_NONE, FIU_SLOT_NONE, FIU_SLOT_NONE,
              FIU_SLOT_NONE, FIU_SLOT_NONE, FIU_SLOT_NONE },
    .i2cMaxRate = 25,
    .spiMaxRate = 40
);
