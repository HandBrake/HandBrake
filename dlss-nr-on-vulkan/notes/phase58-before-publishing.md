# Phase 58 — what a repository takes with it

2026-09-12. The question was whether this is fit to publish. The answer was "not yet,
three things", and this is the two of them that are mine. The third — whether to publish
reverse-engineering output at all — is the owner's and is not decided here.

## It had no licence

Without a `LICENSE` file a public repository is not open source: default copyright means
nobody may use it. **Apache-2.0**, because MLX-DLSS — whose numpy modules this reads at
runtime and whose weight specification it reproduces — is Apache-2.0, and because Apache
carries an explicit patent grant, which is worth having in a project that reimplements a
vendor's inference pass. `NOTICE` states what is not included and credits what is.

## Publishing a repository publishes its history

A file deleted from the tip still ships in every clone, and so does every line edited out
of a file that stayed. `src/tools/publish_check.py` reads the tracked tree on every
`make test`; `make publish-check` walks every blob in every commit. It looks for shapes
rather than names — home directories, mail addresses, mounted volumes, private keys,
tokens, vendor and model file extensions, anything over 200 KB — because the check is
published too, and one that spelled out what it was hiding would be the leak.

It found four things. Three were known:

| | |
| --- | --- |
| `notes/ptx-demangled.txt` | 259 KB of NVIDIA's demangled symbol names, verbatim — **kept, see below** |
| `notes/morning-doa5.md` | a home directory and a Steam library path, in a Russian log for the *other* tree |
| `notes/phase0-acquire.md` | a named community mirror, which tag to take, and how |

The fourth was not: **a 242 KB JPEG made in paint.net, sitting in the object database and
reachable from nothing.** A game frame staged once and never committed. Unreachable
objects are not pushed, so it was never a publication risk — but it is in this clone, and
nobody knew it was there.

## What was removed, and the line it was removed on

The symbol list was taken out on the argument that **a record of what was learned from a
binary is a different thing from a transcription of the binary**, and was then **put back
on the owner's call**, which is the right call: the line does not fall where I first drew
it. Those are 304 C++ symbol names from shared-memory declarations — facts about a file,
regenerable in a minute by anyone holding it, and the evidence that
`notes/ptx-kernel-configs.md` and `notes/MODEL-SPEC.txt` rest on. Publishing an analysis
while withholding what it was derived from makes it unfalsifiable, which is worse than the
thing I was avoiding.

`publish_check.py` carries it as a named exception with the reason written next to it,
rather than by loosening the size limit: the limit exists to catch what nobody meant to
commit, not to forbid what somebody decided to.

`phase0-acquire.md` keeps the half that is useful — how to tell whether the copy in your
hands is NVIDIA's original bytes, by version record, Authenticode chain and section
entropy — and loses the half that is a sourcing guide for a pre-release binary. Two other
notes carried the same pointer and were scrubbed with it.

`morning-doa5.md` is deleted outright: it documented a different tree, in a language the
rest of the repository is not written in, pointed at screenshots that were never
committed, and carried two absolute paths.

## Decided: the recovered architecture is published

The owner's call, made the same day. `docs/ARCHITECTURE.md` is the result — the extent
rule, all sixteen input channels with their exact scaling, the four-channel head and its
composition, the block table, the weight container and the subnormal trap inside it, the
temporal gate with its measured discrimination, and a plain statement of what agreement
with the original is and is not possible.

It exists because "already in the repository" and "usable by someone else" are different
things. The architecture was never removed — only the raw symbol dump was — but it was
spread across `MODEL-SPEC.txt`, `phase3-architecture.md` and a dozen phase notes, in the
order it was *discovered* rather than the order it is *needed*, and at least one section
was still listing as unsolved two things that were solved later. That section now says so
and points here.

`NOTICE` states the position rather than leaving it implied: this is a record of analysis,
no NVIDIA code, no weights, no verbatim transcription, and regenerable by anyone holding
the same file.

## What is still to decide, and it is not a code question

Both removals are only removals from the **tip**. Those bytes remain in the 120 commits
behind it, and publishing the repository publishes them. `make publish-check` says so
every time it is run. The choice is a rewrite that keeps the commit messages and changes
every hash, or a squashed snapshot that keeps neither — and it is one to make knowingly,
at publication, not as a side effect of a hygiene pass.

The remaining question is the history, and only the history.

## The address nobody edits

Publishing a repository publishes the **author and committer of every commit**, and git's
default is whatever is in the local config. Here that was a personal mail address, in all
131 commits across nine branches. It is the one field nobody thinks about and the reason
GitHub hands out `<id>+<login>@users.noreply.github.com`.

`publish_check.py` did not look at it either, until a repository was a command away from
being published with it. It does now: `--history` reports every address that is neither
the trailer git itself writes nor a noreply form, with a count.

That single finding settles the history question, which was otherwise finely balanced.
Every commit has to be rewritten regardless, so keeping the 131 commit messages costs
nothing extra — a squashed snapshot would throw away the chronology for a guarantee the
rewrite already gives.

## The rewrite, and why it is not run from here

`work/publish-rewrite.sh` does three things in one pass over every ref: the address, the
removal of `notes/morning-doa5.md`, and the neutralising of the pointers in the three
notes and the one commit message that carried them. The current files are untouched — the
tip stopped containing any of it two commits ago, so the filters are no-ops there, and the
script verifies that by comparing the tip tree before and after.

It was prepared but not executed from here: rewriting history is destructive, an agent's
safety classifier refused it, and that is the correct outcome — the owner read it and ran
it. A mirror backup sits in `../ProjectsClaude-backup.git` and the script refuses to run
without it.

### What it did, and how that was checked

133 commits across nine branches. Afterwards, on the branches alone — `--all` includes
`refs/original/`, which is filter-branch's own backup and still held the old history, so
the first verification looked clean-until-you-notice-it-was-reading-the-wrong-refs:

| | |
| --- | --- |
| authorship | one address, the noreply form, nothing else |
| `notes/morning-doa5.md` | in 0 commits |
| the pointers, in every blob ever on a branch | none |
| the one commit message | clean |

Then `refs/original` dropped, reflogs expired, `git gc --prune=now`: `.git` fell from
4.1 MB to 1.1 MB, which is the old objects going. `publish_check.py --history` is silent.

The check that matters most is that **nothing else changed**. The tree of the commit at
the last common point is `0377b28b`, byte-identical to the same commit in the mirror taken
before any of this. The rewrite touched the three things it was for and nothing else.
