# Phase 56 — every knob on one screen, and a manual that cannot drift

2026-09-11. The tools were three programs that each knew part of what the daemon could
do, and nothing wrote down what any knob actually *does*. This is the panel, the single
table both the tools and the README render, and the three bugs that writing a test for a
terminal program found.

## One table

`src/layer/nr_knobs.py` is every knob the daemon has: name, range, step, the value the
daemon itself falls back to, a line for a panel and a paragraph for a manual. `nr-ctl`
validates from it, `nr-panel` draws from it, and `src/tools/knob_doc.py` renders it as the
README's knob section — checked by `make test`, so a knob added to the daemon and not to
the documentation is a failing test rather than a surprise for the next reader.

This exists because the drift already happened: `hold` went into the daemon in `phase54`
and never into `nr-ctl`, and nothing noticed until a test compared the two.

The paragraphs quote measurements rather than describing behaviour, because three of the
eight knobs do not do what their names suggest — `colour_strength` is the clearest, where
1.5 is the *most* aggressive setting of the five measured and takes iris saturation from
16.8 to 8.8 (`phase44`).

## The panel

`src/layer/nr-panel`: curses, nothing outside the standard library, arrow keys, and a
status line that is **read from the daemon's own log rather than estimated** — extent,
milliseconds, the history gate, the share of the frame being held still. Chosen over the
GTK4 that is also installed here because an open-source tool that needs only Python works
on the machine it is handed to, and works the same over ssh.

## Three bugs, all in the input path, all invisible without a test

A curses program cannot be checked by eye from a script, so `test_panel.py` runs the real
program on a real pseudo-terminal and presses real keys. It found:

**1. `nodelay` breaks escape sequences, and ESC was bound to quit.** With no delay at all
ncurses hands back the bare ESC of an arrow key before the rest of the sequence arrives.
Every arrow key closed the panel. `halfdelay(2)` lets the sequence assemble; ESC is no
longer a quit key, because it is the first byte of half the others.

**2. `D` was bound to a command, and `D` is the tail of the left arrow.** Binding both
cases of the reset key meant that whenever translation failed, pressing left reset the
selected knob instead. Lower case only.

**3. `model ready in 0.3s` is also "in ... s".** The log parser gated on the seconds
pattern alone, so the model's *load* time was averaged into the frame rate and the readout
lied for the first half-minute after every daemon start. A frame line begins with its
extent; requiring both fixed it.

**4. And the test itself leaked five daemons.** `space` starts the model if nothing is
listening — the point of the feature — so every run left a daemon holding 2.3 GiB of
device buffers on a scratch socket nobody would ever connect to. Four runs took 10 GiB of
15. The test now binds a socket that accepts and closes, which is all `alive()` looks for,
and checks at the end that it left nothing behind.

**5. A short window lost two knobs rather than the explanation.** The layout was a fixed
picture — header, eight rows, a separator, the detail block, a footer at the bottom — so at
twelve rows the knob list ran into the footer and the last two were simply not there. It is
now laid out from the height available: the list and the two footer lines come first, the
explanation gets what is left, and a window too short for even the list says so instead of
ending early.

## And one more test that proved nothing

Sending `ESC [ D` to the pty did not work, and the first diagnosis — that curses was not
translating keys — was wrong. A curses program that calls `keypad` puts the terminal into
**application cursor keys** mode, where the left arrow is `ESC O D`. There is no terminal
emulator on the far side of a pty: the harness *is* the terminal, and it has to send what
the program asked for. `ESC [ D` was passed through raw, which is not what a key press
looks like to the program under test.

That is the second time in two days: `phase55` passed an end-to-end shortcut test by
firing `invokeShortcut`, which dispatches by name and never walks the key map that
crashed. **A harness that does not speak the protocol under test will happily agree with
you.**

## The README

`README.md` is new and is the front door: what this is, that the weights are yours and
never ours, the build, the two processes, the three tools, the knob table, a measured
frame-rate table, how it works in ten lines, and troubleshooting that starts with "look at
the swapchain size before the render scale" — because the passes around the network run at
the output resolution whatever the render scale is, and 1920x1080 is 1.2 fps.
