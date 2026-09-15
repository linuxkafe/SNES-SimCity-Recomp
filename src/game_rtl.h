/*
 * SimCity SNES Recomp — per-game runtime glue header.
 *
 * Declares the GameRunOneFrame, GameDrawPpuFrame, and GameSessionReset
 * entry points that the framework's desktop host calls each frame.
 */

#ifndef GAME_RTL_H
#define GAME_RTL_H

#include "common_cpu_infra.h"

/* GameRunOneFrame:
 *   Run one frame of the guest CPU. The framework calls this at the
 *   target frame rate (NTSC: 60.0988 Hz). The implementation must:
 *   - Deliver NMI at vblank edge (gated on NMITIMEN)
 *   - Run guest in slices until it parks or frame clock expires
 *   - Service raster IRQs when comparator asserts
 *   - Return when frame is complete
 */
void GameRunOneFrame(void);

/* GameDrawPpuFrame:
 *   Rasterize one PPU frame. The framework calls this after
 *   GameRunOneFrame. The implementation must:
 *   - Run HDMA for each line (before visible line)
 *   - Service raster IRQ at comparator line
 *   - Call ppu_runLine for each visible line (0-224)
 */
void GameDrawPpuFrame(void);

/* GameSessionReset:
 *   Called when the user starts a new session (rematch, soft reset).
 *   Must clear any state that must not survive across sessions.
 */
void GameSessionReset(void);

/* Game info structure for the framework. */
extern const RtlGameInfo kGameInfo;

#endif /* GAME_RTL_H */