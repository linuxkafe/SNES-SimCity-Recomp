/* SimCity SNES Recomp — Game-specific RAM variable declarations.
 *
 * The framework includes this by name (snes/cpu.c), so every project must
 * provide one even when it is empty. Populate it as WRAM locations are
 * identified, so generated code and hand-written host code refer to the same
 * address by the same name.
 */

#ifndef VARIABLES_H
#define VARIABLES_H

#include "types.h"

/* SimCity WRAM map (from disassembly) */
/* $7E:0000-$7E:1FFF - Main RAM (mirrored to $00:0000-$00:1FFF) */
/* $7E:2000-$7E:FFFF - Extended RAM */

#endif /* VARIABLES_H */