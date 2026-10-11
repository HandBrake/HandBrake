# The output head, recovered from its padding
2026-09-08

## Localisation

`block70.layer0.layer` is 10,904 elements against 10,336 for a standard C=32 fused
block — **568 extra**, and they are at the **end**: correlation with block1 is 0.840
front-aligned against 0.666 end-aligned, and block70 ends with 132 trailing zeros where
every other fused block ends with 8.

Mirror image of the stem: block0 carries 512 extra at the *front*
(`notes/phase5-stem.md`), block70 carries 568 at the *back*.

## The structure identifies itself

The last 512 elements have a strict period-8 run pattern — `[4 non-zero][4 zero]`
repeating. Reshaped `(64, 8)`:

```
per-column non-zero fraction   [0.5 0.5 0.5 0.5  0. 0. 0. 0.]
rows carrying any data         32 of 64
zeros                          384 of 512, structural
```

Columns 4-7 are entirely zero and only 32 of 64 rows are used. So the stored `(64, 8)`
is a **padded 32 x 4**: the C=32 trunk projected to **four output channels**.

The zeros are not incidental, and the control makes that certain: block0's leading 512
elements contain **not a single zero**. Same size, same model, completely different
population — one is padded, the other dense.

```
out_head       (32, 4)   sd 0.0102   range [-0.0568, 0.0274]
per-channel rms          [0.0080, 0.0051, 0.0095, 0.0157]
blend_scale    scalar    0.73975
```

Three channels are taken as RGB. What the fourth carries is not established; it has the
largest column norm of the four, so it is unlikely to be padding.

## Wiring it in makes the score much worse, and that is expected

```
                                       denoise   verdict
head = first 3 channels (stand-in)     0.05697   PASS-THROUGH
head = recovered 32 -> 4               1.80408   31.36x worse
     + recovered stem as well          8.03393   139.64x worse
```

This is not evidence against the recovery. With the gate in its hardware-confirmed
form the network is a pass-through (correlation with the input +1.0000), so the trunk
reaching the head is essentially the stand-in stem's tiled RGB, scaled. A head trained
to read a 32-channel latent, applied to ten copies of RGB, returns a scramble.

**The score currently rewards the accidental pass-through.** Any correct component
added to a chain whose other end is a stand-in will make it worse. Until the stem is
right, the number is not interpretable, and the `NETWORK-INERT` / `PASS-THROUGH` flags
are the only parts of the output worth reading.

Kept as the default anyway, on the same principle as the gate form: it is what the
weights say, not what the number likes. `--slice-head` restores the stand-in.

## What this says about the stem

The head's padding pins the *output* at 4 channels. The stem is dense — 512 elements,
zero zeros — so it carries no padding to read a shape from, and 16 x 32 remains the
only arithmetic fit for a projection into C=32. But wiring it that way failed, and its
magnitude is wrong for a fan-in of 16 (`notes/phase5-stem.md`). The head being padded
and the stem not being padded is itself a fact worth keeping: whatever the leading 512
of block0 is, it is not the same *kind* of object as the head.
