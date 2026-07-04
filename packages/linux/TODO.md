# TODO — Linux v2.0.0 verification

The v2.0.0 refactor was written on a Windows machine. Everything in the
"Verified in WSL2" section was actually executed inside WSL2 (Ubuntu,
systemd enabled), but WSL has no real input devices and no `/dev/uinput`,
so the key-mapping itself still needs a real Linux machine.

## Needs verification on real hardware

- [ ] Build from the repo root: `cmake -S . -B build && cmake --build build`
- [ ] `sudo cm on` → pressing/releasing Caps Lock maps to left click down/up
- [ ] Dragging with Caps Lock held stays one continuous drag (no repeated clicks)
- [ ] Normal typing is unaffected; Caps Lock toggle/LED never engages while running
- [ ] Shift still produces capitals
- [ ] `sudo cm off` stops the mapper and the keyboard returns to normal (grab released)
- [ ] `sudo cm startup --add` → reboot → mapper is active
- [ ] `sudo cm startup --remove` → reboot → mapper stays inactive
- [ ] Multiple keyboards (e.g. laptop + USB) are all mapped
- [ ] Release packaging: tar.gz containing `cm`, `cm_runner`, `install.sh`, `uninstall.sh`

## Verified in WSL2 (2026-07-05)

- Clean build with gcc 15.2, `-Wall -Wextra`, zero warnings
- `cm --help` / `--version` / `status` / invalid input, including exit codes
  (0 on success, 1 on failure/invalid)
- Argument order flexibility (`cm --add st` == `cm st --add`)
- Root guard on `on` / `off` / `startup`
- `install.sh` / `uninstall.sh` round trip
- Daemonization plumbing: fork → setsid → double fork → execv, PID file
  written by the grandchild; a stale PID is correctly reported as not running
- `startup --add/--remove` against a live systemd: unit file written,
  enabled, started, then stopped, disabled, removed

## After the v2 release

- [ ] Update `packages/linux/README.md` — it intentionally still documents
      v1. The only user-facing change is the command rename
      `service | srv` → `startup | st`, plus the CMake build note above.
