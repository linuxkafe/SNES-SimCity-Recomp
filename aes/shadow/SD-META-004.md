---
id: SD-META-004
title: "Architectural Decision Framework for SNES Ports"
synthesis: "For SNES ports without coprocessors, clean-room reimplementation with runtime asset extraction dominates static recompilation on: license (permissive vs non-commercial), toolchain simplicity (C++ vs Rust+Python), bug control (fix vs preserve), maintainability (owned vs generated), and performance (native vs emulated). Recompilation only wins when coprocessor microcode is infeasible to reimplement (Super FX, SA-1, DSP)."
pointer:
  path: "aes/tickets/T026-learn.md"
  content_hash: ""
epistemic_state: SUPORTADA
score:
  activation: 0.0
  pinned: false
  centrality: 1.0
provenance:
  generated_by: "T026 (aes-learn)"
  generated_at: "2026-09-14"
  source_ticket: "T026"
  source_conversation: ""
created: "2026-09-14"
last_accessed: "2026-09-14"
---