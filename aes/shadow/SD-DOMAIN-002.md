---
id: SD-DOMAIN-002
title: "SNES BG Mode 1 Title Screen Architecture"
synthesis: "The SimCity SNES title screen uses BG Mode 1 with three layers: L3 (2bpp atmosphere), L1 (4bpp skyline), L2 (4bpp logo). All graphics LC_LZ5 compressed at known CPU addresses; palette is raw BGR555 (128 colors = 8 sub-palettes × 16). Tilemaps use standard SNES attributes (10-bit tile ID, 3-bit sub-palette, hflip/vflip). This architecture is distinct from the city view (which uses Mode 1 with different layer assignments)."
pointer:
  path: "aes/tickets/T021-learn.md"
  content_hash: ""
epistemic_state: SUPORTADA
score:
  activation: 0.0
  pinned: false
  centrality: 1.0
provenance:
  generated_by: "T021 (aes-learn)"
  generated_at: "2026-09-14"
  source_ticket: "T021"
  source_conversation: ""
created: "2026-09-14"
last_accessed: "2026-09-14"
---