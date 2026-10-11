# Phase 55 — one switch on a key, and the compositor I took down with it

2026-09-11. `nr-ctl` is a console, and a console needs the game to lose the screen. This
is the other half: `src/layer/nr-toggle`, a single flip with a desktop notification for an
answer, small enough to hang off a key. The tool is good. **The way this note originally
had it bound crashed KWin on the first keypress**, and that is the part worth reading.

> **Withdrawn, and kept because it is the trap:** the first version of this note said that
> a Plasma global shortcut can be installed by writing `kglobalshortcutsrc`, registering
> over kglobalaccel's D-Bus interface, and stopping `plasma-kglobalaccel.service` to undo
> it. Every one of those steps ran, `allComponents` listed the component, firing it through
> `invokeShortcut` worked — and the first real keypress killed the session. The sections
> below say why. `nr-toggle install` no longer registers anything.

## What happened

```
#4  Component::uniqueName() const                  libKGlobalAccelD.so.0
#5  GlobalShortcutsRegistry::processKey(int, ...)  libKGlobalAccelD.so.0
#6  kwin_wayland + 0x704ac
    SIGSEGV
```

A key was pressed, the registry walked its map, and dereferenced a `Component` that was
not there any more. The session went down and restarted.

## The fact everything else follows from

**In Plasma 6.7 the global shortcut registry lives inside KWin.** `busctl --user status
org.kde.kglobalaccel` answers `Comm=kwin_wayland`, and `systemctl --user is-active
plasma-kglobalaccel.service` answers **inactive**. That unit exists and is not running.

So the whole lifecycle in the withdrawn version was aimed at the wrong process:

- `systemctl --user stop plasma-kglobalaccel.service` stopped nothing.
- deleting the launcher `.desktop` and rebuilding the service cache removed what a live
  component in KWin was pointing at.
- editing `kglobalshortcutsrc` edited a file the live registry reads only at its own
  start and **writes back from memory**, which is why the entries kept reappearing and
  why the tool's own `status` reported "not bound" on a binding that had just fired.

Install, uninstall, install: by the third round the registry held a component whose file
was gone. `invokeShortcut` never touched the broken path — it dispatches by name — so the
end-to-end test passed. `processKey` is the path a real key takes, and it is the one that
died. **A test that exercises the mechanism around the thing under test proves nothing
about the thing under test.**

Which of the two possible mechanisms it was — a `KService` invalidated under the
component by `kbuildsycoca6`, or a component torn down when the short-lived `busctl` that
registered it dropped off the bus — is not settled here, and settling it would cost
another session. It does not change what to do. KDE's registry should not segfault on
either; handing it an unsupported lifecycle was ours.

## What the tool does now

Nothing that can reach the registry. `nr-toggle install` prints the two commands and opens
System Settings -> Shortcuts, which binds them through the path KDE supports. Half a minute,
once. `status` finds the key again by reading the `Exec` line of whatever launcher System
Settings wrote — the only reliable link back, since it names the shortcut after its own file.

The binding is still the compositor's job and cannot be anything else: on Wayland an
application cannot grab a key for itself, `/dev/input/event*` needs the `input` group this
account is not in, and neither python-evdev nor tkinter is installed here.

## Two things the tool does because a key is not a terminal

**Turning it on starts the daemon.** A toggle that answers "no daemon, go and start one"
sends the user to the terminal it exists to avoid. One press from nothing is under a second
— the model is in page cache after the first load — and the daemon refuses to start when
one is already listening, so a second press is harmless. Turning it *off* leaves the daemon
up on purpose: the trigger is separate from the model precisely so the picture can come and
go without paying the load again. With no settings file at all it writes
`render_scale = 0.55`, this project's measured compromise (`phase51`, `phase52`); anything
already chosen wins.

**Asking whether the daemon is alive no longer looks like a failure.** The probe is a
connect and a close, and the daemon logged every one as `frame rejected/failed` — every
status line and every press. `receive(..., probe_ok=True)` now separates "closed before a
byte" from a truncated frame; a truncated frame still reports.

## What was shared rather than copied

`nr-ctl` and `nr-toggle` have to agree on three paths and on how the settings file is
written. They were in `nr-ctl` first, so they moved to `src/layer/nr_paths.py` rather than
being copied. `test_toggle.py` checks the two tools resolve to the same files — and,
separately, that `nr-ctl` can reach **every** knob the daemon has, which it could not:
`hold` went into the daemon in `phase54` and never into the control tool. The same test
found `start_daemon` passing `--settings` without `--socket`, which would have started a
daemon nothing was talking to whenever `NR_LAYER_SOCKET` was set.
