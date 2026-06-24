# DOAX fiber / scheduler / boot hooks archive (2026-06-24)

Snapshot of the full hook layer removed for a clean bring-up restart.
Use these files as a reference when re-adding behavior incrementally.

## Files

| File | Contents |
|------|----------|
| `doax_manifest.toml` | All `DOAX_*` named hooks + 4 midasm skips/guards |
| `doax_hooks.cpp` | ~1400 lines: fiber swap override, scheduler drain, menu-kick, travel guards, boot fiber, movie/input probes |
| `doax_hooks.h` | `InstallDoaxGuestPcFiber` declaration |

## Hook categories (grep strings in `doax_hooks.cpp`)

- **Guest fiber:** `DOAX_FiberContextSwitch`, `InstallDoaxGuestPcFiber`, GPR callee-save preserve
- **Scheduler:** `DOAX_SchedulerDrainDispatch`, `DOAX_SchedulerDrainWake`, `DOAX_SchedulerFiberSwap`, `menu-kick`, `flag0-safety`
- **Work queue:** `DOAX_WorkQueueDispatchLoop`, `DOAX_WorkQueueSlotWake`, `sub_8258CDF8`
- **Boot / movies:** warning/ninja midasm, boot work fiber, `BootPresentStateUpdate`, `PlayMovie` probes
- **Menu / travel:** `DOAX_MenuItemConfirm`, scene transition, island load, overlay guards, fade timeline
- **Input:** `sub_8274B650`, `sub_82782BF0`, `sub_82782B58` probes

## Key guest globals (IDA)

| Address | Role |
|---------|------|
| `0x833B8DF8` | Scheduler flags base |
| `0x833BB763` | Boot present byte (`5` = main-menu-ready) |
| `0x833B84C8` | Present state index |
| `0x833B8DEB/DEC/DEF` | Menu work-fiber state |

## Related docs (repo)

- `docs/DOAX-fiber-notes.md`
- `docs/DOAX-warning-skip-attempts.md`

## Restore workflow

1. Diff `doax_manifest.toml` / `doax_hooks.cpp` against this archive.
2. Re-enable one hook group at a time; codegen + rebuild after manifest changes.
3. Do not edit `DOAX/generated/` permanently — manifest/TOML or `doax_hooks.cpp` only.
