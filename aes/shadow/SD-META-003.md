---
id: SD-META-003
title: "Runtime Asset Extraction Pattern"
synthesis: "The pattern 'disassembly address → LoROM translate → LC_LZ5 decompress → format decode → compose → cache' works uniformly for: scenario maps (T004), city tiles (T017), title screen (T021). Each adds domain-specific decode (4bpp/2bpp, tilemap dimensions) but shares the extraction pipeline. This pattern should be formalized as a reusable RomAssetExtractor for future assets (font T019, building sprites T023, audio T010)."
pointer:
  path: "aes/tickets/T021-learn.md"
  content_hash: ""
epistemic_state: HIPÓTESE
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