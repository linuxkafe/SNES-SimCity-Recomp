# SimCity SNES Recomp – Host/Guest Loop Model

## Host frame loop – src/game_rtl.c GameRunOneFrame

```mermaid
flowchart TD
    HStart[Start Frame]
    HBoot{booting? g_resume_pc==0}
    HReset[Load reset_vector 0xFFFC]
    HNMI{NMI enabled? g_snes->nmiEnabled}
    HDeliverNMI[Deliver NMI game_run_interrupt nmi_vector]
    HSlice[Slice loop: interp_bridge_run_until_quiescent<br/>until master_cycles >= frame_end or parked]
    HIRQ{g_snes->inIrq && !I flag}
    HServiceIRQ[Service IRQ game_run_interrupt irq_vector]
    HWAI{interp_bridge_lle_took_wai?}
    HDraw[GameDrawPpuFrame - HDMA + PPU lines]
    HEnd[End Frame]
    HStart --> HBoot
    HBoot -->|yes| HReset --> HNMI
    HBoot -->|no| HNMI
    HNMI -->|yes| HDeliverNMI --> HSlice
    HNMI -->|no| HSlice
    HSlice --> HIRQ
    HIRQ -->|yes| HServiceIRQ --> HSlice
    HIRQ -->|no| HWAI
    HWAI -->|yes| HDraw --> HEnd
    HWAI -->|no| HSlice
```

Key invariants from `src/game_rtl.c`:
* One NTSC frame = `357368` master cycles.
* NMI delivered at vblank edge gated on `NMITIMEN`.
* Resume PC `g_resume_pc` is host state, saved/loaded via `SimCityStateSaveExtra/LoadExtra`.
* NMI preserve regs active by default `SNESRECOMP_NMI_PRESERVE_S=1` to avoid stack relocation bug T050.

## Guest main vblank wait – $930D

```mermaid
flowchart TD
    GStart[Main loop entry]
    GSEP[SEP #$20]
    GSTZ[STZ $00B9 clear vblank token]
    GINC[INC $00C7 frame counter]
    GLDA[LDA $00B9]
    GBEQ{BEQ $9311?}
    GProcess[ProcessGameMode - simulation step]
    GRTS[RTS]
    GStart --> GSEP --> GSTZ --> GINC --> GLDA --> GBEQ
    GBEQ -->|yes| GStart
    GBEQ -->|no| GProcess --> GRTS --> GStart
```

NMI handler sets token `INC $00B9` – early exit branch $80B2/$80BC. Token is the vblank handshake.

## Current problem

* City loads, animation runs, date stays 1900 JAN.
* Measured on Steam Deck headless: 60.03 fps, guest 2.45 ms/frame, WRAM changes 193 bytes over 600 frames.
* Live city clock not verified on Deck; requires `scripts/d_city.script` + HUD date crop verification.

See `docs/RE_CITY_FREEZE.md` for full hostile analysis and measurement history.
