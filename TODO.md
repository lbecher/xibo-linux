# TODO

- [x] Save player local/config files under the user config directory
- [x] Migrate libgtkmm-3.0-dev to libgtkmm-4.0-dev

## Windows (C#) vs Linux (C++) parity roadmap

### Goal
- [ ] Reach feature and behavior parity for day-to-day playback between Linux player and Windows player.
- [ ] Keep Linux implementation stable on GTK4 and aligned with CMS expectations (XMDS/XMR/cache).

### P0 (Critical - blocks real-world parity)

#### 1) Embedded server actions parity (`/trigger` and `/duration`)
- [x] Implement real action execution pipeline on Linux (not only logging counters). (`next`, `previous`, `navLayout` by id/prefix, `duration/expire`)
- [x] Support action paths equivalent to Windows behavior (`next`, `previous`, `nav*`, `widget`, `command`, duration-expiry flow). (implemented via embedded trigger parser)
- [x] Wire schedule/layout manager integration so actions affect the active timeline immediately.
- [x] Add structured logs for action input, resolution, and execution result.
- [x] Add tests (or scripted validation) for each trigger type. (added scheduler/queue trigger tests)
Acceptance:
- Triggering the same action sequence on Windows and Linux yields the same visible behavior and state transitions.
Remaining scope notes:
- [x] Implement `navWidget` behavior. (trigger code format: `navWidget:<widgetId>` or `navWidget:<regionId>:<widgetId>`)
- [x] Implement `command` action behavior. (trigger code format: `command:<shell command>`, gated by `enableShellCommands`)
- [x] Implement `duration/extend` and `duration/set` behavior by source widget/region.

#### 2) XMR actions + transport parity
- [x] Add missing action handlers on Linux: `commandAction`, `dataUpdate`, `triggerWebhook`, `purgeAll`.
- [x] Keep existing handlers (`changeLayout`, `overlayLayout`, `revertToSchedule`, `screenShot`, etc.) behavior-compatible.
- [ ] Add WebSocket XMR support parity where Windows supports `XmrType=ws`.
- [x] Add retry/backoff and reconnect behavior matching current Windows expectations. (ZMQ reconnect interval + max interval configured)
- [x] Add logs with action id, payload summary, and execution outcome.
Acceptance:
- CMS-issued XMR commands that work on Windows execute successfully on Linux with equivalent results.
Remaining scope notes:
- [ ] Implement full WebSocket transport mode (`XmrType=ws`) handshake/channel flow equivalent to Windows.

#### 3) GTK4 rendering correctness (layout geometry and scale)
- [ ] Fix coordinate mapping to keep all regions/media at correct position/size.
- [ ] Validate DPI/scaling handling for fullscreen and windowed mode.
- [ ] Ensure overlays and fixed containers preserve expected z-order and clipping.
- [ ] Re-validate text, image, and logo positioning against reference screenshots.
- [ ] Remove/mitigate GTK warnings caused by invalid widget size requests where possible.
Acceptance:
- Linux screenshot matches Windows reference layout for default test layouts (no tiny-corner media, no shifted canvas).

#### 4) Cache/hash validation compatibility
- [x] Align Linux file hash verification with CMS/Windows canonical behavior (same algorithm/normalization/input bytes). (hash now calculated from bytes persisted to disk)
- [x] Document and handle known cases that can change file bytes after collection (if any). (`updated`/timestamp-backed cache entries skip startup hash re-check)
- [x] Add diagnostics to show hash source, expected hash, computed hash, and normalization flags.
- [ ] Add regression checks for `1.html`, `2.html`, and global dependants (example: `bundle.min.js`).
Acceptance:
- No false-positive "cache hash mismatch" on freshly collected files that are valid in Windows player.
Remaining scope notes:
- [x] Verify whether resource HTML files (`*.html` with `updated` field) should be hash-validated at startup or only timestamp-validated. (timestamp-validated entries now bypass startup hash check)
- [ ] Add a startup regression scenario that reproduces and validates the `1.html`/`2.html` mismatch fix.

### P1 (High - major feature gaps)

#### 5) Media type support expansion
- [ ] Add missing media/parser support where applicable: `shellcommand`, `htmlpackage`, `spacer`.
- [ ] Decide and document Linux strategy for legacy Windows-only types (`powerpoint`, `flash`) with explicit fallback behavior.
- [ ] Keep current media (`image`, `video`, `audio`, `webpage`, `embedded`, `ticker`, etc.) behavior-compatible.
- [ ] Add capability matrix in docs with "supported / fallback / unsupported".
Acceptance:
- Layouts using supported media types render and play on Linux as they do on Windows, with explicit fallback for unsupported legacy types.

#### 6) Dynamic data and weather agent parity
- [ ] Implement Linux equivalents for data/weather collection flows used by widgets.
- [ ] Extend XMDS sender/collection interval with missing endpoints and scheduling logic.
- [ ] Cache and expiry policy for data/weather responses should match Windows semantics.
- [ ] Add observability logs and failure handling (timeouts, stale data, retries).
Acceptance:
- DataSet/Data-driven and weather widgets refresh correctly on Linux under same CMS configuration.

#### 7) Schedule model parity (advanced behaviors)
- [ ] Expand Linux schedule model for missing behaviors (actions, interrupt/share-of-voice, cycle/max-plays, geo where applicable).
- [ ] Validate priority/conflict resolution order against Windows scheduler.
- [ ] Ensure overlay/revert/interrupt interactions are deterministic and test-covered.
Acceptance:
- Same schedule XML + same runtime events produce same layout selection order on Linux and Windows.

### P2 (Important - full product parity and operability)

#### 8) Settings/config parity
- [ ] Expand `PlayerSettings` and `RegisterDisplay` handling to include missing CMS-driven options.
- [ ] Add options UI coverage for newly supported settings.
- [ ] Keep defaults safe and backward-compatible for existing Linux installs.
Acceptance:
- CMS can manage Linux player options at parity level expected by Windows deployments.

#### 9) Platform behavior parity and hardening
- [ ] Compare watchdog/restart behavior with Windows and fill critical gaps.
- [ ] Validate kiosk-like behavior expectations where relevant on Linux desktop environments.
- [ ] Add startup and recovery tests (network down/up, CMS unavailable, stale cache, invalid layout).
Acceptance:
- Linux player recovers predictably in operational scenarios commonly validated on Windows deployments.

#### 10) Parity test suite + documentation
- [ ] Build a parity checklist test pack (manual + automated where possible).
- [ ] Add reproducible test fixtures: schedule XML, media bundle, XMR scripts, expected screenshots/log snippets.
- [ ] Document known intentional differences between platforms.
Acceptance:
- Team can run a repeatable parity validation before releases and track regressions quickly.
