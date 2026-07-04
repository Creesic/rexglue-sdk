# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-07-03

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **Project:** ReXGlue080plume
- **Description:** > [!CAUTION]

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->

## Key Learnings (2026-07-03 session 6)
- IDA MCP server mapping: ida37 = FM2 (FM2.xex.i64), ida38 = Sonic Unleashed, ida39 = MM3. Check server_health first.
- Handoff-doc mechanism models can be wrong: pt10's "retire gated on a D3D fence" was disproven by decompiling the counter writers. Before building a fix on a counter's meaning, find its writers in IDA (py_eval scan for stores at the displacement, remembering embedded structs shift displacements: pool at renderThread+2224 => +120 becomes 0x928, at wrapper+24 => 0x90).
- FM2 deferred pool pacing: game loop is vblank-locked via PulseEvent(829C24C0); plume replaces it with StartGatePulseThreadOnce at fm2_plume_gate_pulse_hz (default 1000 = uncapped producer). Any backlog/drop symptom in the 0x4001CA20 pool should FIRST be checked against the pulse rate.
- fm2 --log_file TRUNCATES (recreates) the target file, it does not append. Offset-based tailing from a pre-launch length is wrong across launches; use separate log files per run instead.

## Do-Not-Repeat (2026-07-03)
- Do NOT re-run the fm2_plume_gate_pulse_hz pacing A/B (60/30 Hz) expecting textured UI: user-verified NO visual difference; default stays 1000. The pool/pacing mechanism findings (see docs/FM2-handoff-2026-07-02-session5.md session 6) remain valid telemetry, but pacing alone does not fix the black textured UI.
