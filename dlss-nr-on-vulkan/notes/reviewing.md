# Reviewing this repository

`/ultrareview` needs a base branch and a diff under its size limit. This repository has
a single line of history on `master`, so a first review of the whole tree is **180 files
and 25 903 insertions** and is refused. Four base branches cut it into areas: each one
is `master` with one area removed, so `master` against it shows exactly that area as
additions.

```sh
git checkout review-layer && /ultrareview pre-layer && git checkout master
```

and the same for `gpu`, `ref`, `tools`. **Check the scope line the command prints
against this table** — that is how both earlier mistakes announced themselves:

**File count is the check**, not the line count: lines drift with every commit, files do
not, and the file count alone catches both mistakes below — they showed as *1 file* and
as *181 files*.

| checkout | base to pass | files | reviewed |
|---|---|---|---|
| `review-layer` | `pre-layer` | 11 | 2026-09-10, `notes/phase39-layer-review.md` |
| `review-gpu` | `pre-gpu` | 29 | 2026-09-10, `notes/phase40-gpu-review.md` |
| `review-ref` | `pre-ref` | 15 | not yet |
| `review-tools` | `pre-tools` | 43 | not yet — `src/tools`, `src/bench`, `src/probe` |

Each `review-*` branch has **master's exact tree**, so checking one out changes no file
on disk, and the reviewer can read the whole codebase. Its parent `pre-*` is an orphan
holding the same tree with that one area removed, so the single commit between them
carries exactly the area and nothing else.

That equality is the whole trick, and it stops holding the moment master moves. Rebuild
all four pairs after any commit — `src/tools`, `src/bench` and `src/probe` go together in
`tools`:

```sh
export GIT_INDEX_FILE=$(mktemp -u)
full=$(git rev-parse master^{tree})
for area in layer gpu ref tools; do
    case $area in tools) dirs="src/tools src/bench src/probe";; *) dirs="src/$area";; esac
    rm -f "$GIT_INDEX_FILE"; git read-tree master; git rm -r -q --cached $dirs
    base=$(git commit-tree $(git write-tree) -m "Everything except $dirs.")
    git branch -f "pre-$area"    "$base"
    git branch -f "review-$area" "$(git commit-tree "$full" -p "$base" -m "$area, for review")"
done
rm -f "$GIT_INDEX_FILE"; unset GIT_INDEX_FILE
```

Plumbing, so the working tree is never touched and `master` is never left.

### Two wrong ways to build this, both tried

The tool reviews **the patches of the commits unique to the branch**, not a tree diff.
That is not obvious and it invalidated two attempts:

1. **Bases as descendants of master** — take master, delete the area, commit. `git diff`
   shows the area as additions and looks right. But the only commit master had that such
   a base lacked was the one adding this file, so the scope came out as *1 file, 65
   insertions* and a review was spent on the reviewing instructions.
2. **Orphan branches holding only their own area.** The scope was then correct, but the
   tree was not: a reviewer on such a branch sees `src/layer` and nothing else. That
   matters — the one substantive finding from the wasted review was that two tests in
   `src/gpu` import from files in `src/ref`, which is invisible from a branch holding
   only one of them. Running `/ultrareview review-layer` *from master* also failed here,
   with 181 files, because an orphan shares no history and every commit in master is
   then unique to it.

Always pass a base: bare `/ultrareview` looks for `main`, which does not exist, and
creating one would silently make some arbitrary scope the default.

**All three free reviews are spent** (2026-09-10): `layer` twice — the first on a
wrongly-built branch — and `gpu`. `review-ref` and `review-tools` are built and correct
but have not been reviewed; they need a paid run or another allocation.

## What is worth a reviewer's time, in order

**`review-layer` — `src/layer`, and the reason to start here.** It is C that loads
*inside another process*, in this case a game running under Wine. It intercepts
`vkQueuePresentKHR`, copies swapchain images, talks to a daemon over a Unix socket, and
hands the result back. A bug here does not produce a wrong picture; it takes the host
game down. It has already had one such bug fixed — `write()` instead of
`send(..., MSG_NOSIGNAL)`, where a daemon dying mid-transfer would kill the game with
SIGPIPE. Look at lifetimes, partial reads and writes, the two ABIs, and what happens
when the daemon is absent, slow, or lying about sizes.

**`review-gpu` — `src/gpu`.** The resident runtime: `libxmx.c` owns the Vulkan device,
buffers and pipelines and records whole frames; the `.comp` shaders are the graph. The
sharpest things to check are the ones this project got wrong before: buffer sizing and
offsets around `xmx_rec_gemm` (cooperative-matrix loads are **not** bounds-checked here,
`cooperativeMatrixRobustBufferAccess` is false, so an overrun is silent), the scratch
arena's aliasing plan against the barriers in `nr_resident.py`, and whether every
`coopMatStore` writes into an array of the matrix's own component type — the one rule
whose violation cost this project three phases (`notes/phase38-there-was-no-bug.md`).

**`review-ref` — `src/ref`.** The CPU reference and the frame/temporal drivers. The live
files are `nr_model.py`, `nr_frame.py`, `nr_temporal.py`, `nr_display.py`, `nr_accel.py`,
`image_io.py` and their tests.

`hnet_model.py`, `hnet_ops.py`, `hnet_ref.py`, `forward.py` and `run_frame.py` are
**superseded** — they decode the weight container the wrong way — and are kept for the
PTX-derived findings they encode. They are not worth reviewing *as production code*, but
"do not read them" was too strong, and a review said so: **two tests in `src/gpu` import
from them** — `test_attention_gpu.py` takes `Model`, `softmax`, `l2_normalize`,
`HEAD_DIM`, `TOKENS` and `GQA_RATIO`, and `test_layer.py` takes `Model`. Whoever reviews
`review-gpu` will meet those imports and needs to know what is behind them.

### The two tests outside `make test`, and why

Both load `work/weights_ht.bin`, which is carved out of the DLL and gitignored, so
neither can run on a fresh clone — that alone keeps them out of the recipe. Beyond that
they differ, and the difference matters:

- `test_attention_gpu.py` **passes**: worst relative deviation **2.8e-04**, Phase 4's
  acceptance result. It asks whether the GPU path reproduces the CPU path *on the same
  weights*, so a wrong decode does not invalidate it. It is a kernel test.
- `test_layer.py` **fails at 0.22 and cannot pass.** The dense-FP16 decode yields values
  including FP16 subnormals; XMX flushes subnormal operands to zero and the float64
  reference does not. The premise it was written under — "27 % of this model's parameters
  are subnormal" — was an artefact of the same wrong decode, and the real figure is
  0.00006 %.

Both now say this at the top of the file, so nobody runs them expecting green.

**`base-tools` — `src/bench`, `src/probe`, `src/tools`.** Measurement and
reverse-engineering tooling. Lowest risk, but the benchmarks are where claims come from,
so an error here is an error in the notes. Worth checking that each one measures what
its name says: several findings in `notes/` turned on a benchmark's warm-up, its pairing,
or its expectations being wrong rather than the code under test.

## What not to review

`notes/` is 7 239 lines of prose and is the project's record, including its wrong turns,
which are marked as such. It is not code and it will consume a reviewer for nothing.

## Ground truth for "is this still correct"

```sh
make test          # 95 checks, including bit-exactness against the CPU reference
```

The number that matters is the resident graph's correlation with the host reference,
**0.981311**. It has not moved through any optimisation in this repository, and a change
that moves it is either a bug or a deliberate, documented trade.
