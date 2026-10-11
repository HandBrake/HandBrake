"""Every knob the daemon has, described once.

The control tool, the panel and the documentation all render this table, so a knob
cannot exist in one and be missing from another — which has happened: `hold` went into
the daemon and never into `nr-ctl`, and nothing noticed until a test compared the two.

`summary` is what a panel shows next to the value. `detail` is what a user choosing a
value needs: what the knob does, what the ends of its range do and where to start. Both
are general — no scene or game a reader cannot see. The measurements behind them live in
`notes/`, where they can be checked.
"""
import collections

Knob = collections.namedtuple("Knob", "name label kind low high step default summary detail")

PROFILES = ("standard", "natural", "cinematic", "neutral")

KNOBS = (
    Knob(
        "render_scale", "render scale", "number", 0.05, 1.0, 0.05, 1.0,
        "the fraction of each side the network runs on",
        "The only knob that changes the frame rate. The network draws its detail on a "
        "frame this much smaller; the detail is then scaled up and laid over the game's "
        "full-resolution frame, so the game's own pixels are never resampled. Lower is "
        "faster and draws coarser detail, at about the same strength. The network's frame "
        "is never smaller than 320 pixels a side (`min_extent`), so on a small window a low "
        "scale is raised, at no cost, to fill the cheapest frame it lands on: 640x360 runs "
        "as 0.5 for anything up to 0.5, 800x450 as 0.4 for anything up to 0.4, and a lower "
        "scale is never the slower one. For play, 0.35-0.6 is the useful range. For "
        "screenshots, 1.0 hands the network the game's raw pixels, jagged edges and all, "
        "and it turns a little of them into pixel-level grain; 0.9 smooths them first and "
        "draws the same strength of effect. Cost on an Arc 140V: about 130 ms per megapixel "
        "of network frame, plus 10-20 ms that grows with the game's own resolution.",
    ),
    Knob(
        "min_extent", "min extent", "number", 128.0, 320.0, 64.0, 320.0,
        "the smallest side the network's frame is padded to",
        "The network's frame is padded, by mirroring the picture, to at least this many "
        "pixels on a side. 320 is what NVIDIA's own driver does; the network itself runs "
        "down to 128. At small live sizes most of a 320 frame is padding, so a lower floor "
        "is much faster — on an Arc 140V, 512x288 at scale 0.35 takes 25 ms a frame at "
        "320 and 14 at 128 — and draws a somewhat different picture, since the network no "
        "longer sees a mirrored copy of the scene around it. Neither is wrong; compare them "
        "in a game. It changes nothing once the scaled frame is larger than this anyway.",
    ),
    Knob(
        "profile", "profile", "choice", None, None, None, "standard",
        "which way to trade skin texture against highlights, and its colour grade",
        "The style the network is asked for, and the colour grade that comes with it. The "
        "profiles are a trade, not a quality ladder: what one adds to skin and surface "
        "texture it takes from highlights. `standard` is the default and adds the most "
        "texture; `natural` and `cinematic` keep more of the highlights, and on very bright "
        "scenes `cinematic` can smooth fine detail rather than add it. After the network, "
        "as NVIDIA grades them, `natural` comes out a little darker, flatter and less "
        "saturated and `cinematic` less saturated. `neutral` all but switches the effect "
        "off. The profile is an input to the network, so it takes effect on the next frame "
        "the network draws; the knobs below act after it.",
    ),
    Knob(
        "intensity", "intensity", "number", 0.0, 2.0, 0.05, 1.0,
        "how far to go towards the model's picture, or past it",
        "Blends the model's picture with the game's own. 1 is the model's picture, 0 is the "
        "game's frame untouched, and values between are part of the way. Above 1 the "
        "blend goes past the model and exaggerates what it changed, which can look "
        "overdone. Free to change: nothing is re-run.",
    ),
    Knob(
        "detail_strength", "detail strength", "number", 0.0, 2.0, 0.05, 1.0,
        "how strongly to apply the fine detail the pass adds",
        "The change the pass makes is split into fine detail — pores, hair, grain — and "
        "broad tone. This scales the fine part: 0 keeps only the tonal change, above 1 "
        "sharpens further, and past about 2 it over-sharpens. Away from 1 it costs one "
        "blur over the whole frame.",
    ),
    Knob(
        "colour_strength", "colour strength", "number", 0.0, 2.0, 0.05, 1.0,
        "how strongly to apply the pass's change of tone and colour",
        "The broad half of the same split: how much of the pass's change in tone and "
        "colour is applied. It does not add colour back. The pass tends to calm bright, "
        "saturated areas, and this scales that change — so above 1 colours can look "
        "more washed out, not richer, and 0 keeps the game's own tone with the detail "
        "on top.",
    ),
    Knob(
        "temporal", "temporal", "number", 0.0, 1.0, 0.05, 1.0,
        "how much of the previous frame to carry over",
        "Each frame, the previous result is fed back to the network, which decides per "
        "pixel how much of it to keep; that is what keeps the picture from shimmering. "
        "This scales how much it keeps. 0 draws every frame on its own and forgets the "
        "stored frame, so turning it back on cannot bring back an old one. Costs a few "
        "per cent of the frame time.",
    ),
    Knob(
        "hold", "hold", "number", 0.0, 1.0, 0.05, 1.0,
        "how firmly to keep areas the game did not change",
        "Where the game hands back exactly the same pixel as last frame, the previous "
        "result is still right for it, so it is kept at least this firmly. This is what "
        "stops still areas — backgrounds, a waiting character — from shimmering while "
        "something else moves. It cannot leave a trail: the first frame in which the game "
        "changes a pixel lets go of it.",
    ),
    Knob(
        "release", "release", "number", 0.0, 64.0, 1.0, 24.0,
        "change, in levels of 255, that drops the previous frame where something moved",
        "The other side of `hold`. Where the game's own pixel changed by this many levels "
        "of 255 or more, something moved there, and the previous result is dropped for "
        "that pixel; smaller changes keep a share that falls with the change. It is what "
        "prevents trails behind moving objects, since the layer has no motion vectors to "
        "follow them with. 16-24 is the useful range: lower drops more and the picture "
        "starts to shimmer over moving things, 0 turns it off and trails come back.",
    ),
    Knob(
        "cut_limit", "cut limit", "number", 0.0, 1.0, 0.01, 0.15,
        "how big a change between frames counts as a new scene",
        "The average change between two frames above which the scene is taken to have cut "
        "— a camera cut, a menu, a replay — and the previous frame is dropped entirely "
        "instead of pixel by pixel. Lower cuts more readily; 1 never cuts.",
    ),
)

BY_NAME = {knob.name: knob for knob in KNOBS}
# What the daemon itself falls back to with no settings file. `test_toggle.py` checks
# these against its argument parser, because a panel that shows a wrong "current" value
# is worse than one that shows none.
DEFAULTS = {knob.name: knob.default for knob in KNOBS}

# The whole round trip, median of nine frames each, measured by `src/bench/live_rates.py`
# on 2026-09-26 — the daemon's own cost, with no game competing for the GPU. `nr-ctl rates`,
# the panel and the README all read this one table; the README's copy is generated from it
# by `src/tools/knob_doc.py`, because the hand-written one went two days out of date the
# moment the host passes moved to C and then stayed wrong for a week.
RATES = (
    (512, 288, 0.35, 25.9),
    (512, 288, 0.50, 26.0),
    (640, 360, 0.35, 26.0),
    (640, 360, 0.50, 27.0),
    (854, 480, 0.50, 34.0),
    (1024, 768, 0.55, 56.5),
    (1920, 1080, 0.55, 111.8),
)
RATES_MEASURED = "2026-09-27"
# What a reader of the table needs and the numbers cannot say. Empty when there is nothing.
RATES_NOTE = ("Medians of six runs with swap empty; a row's runs spread up to 15 %. On "
              "2026-09-23, with 5.5 GiB in zram and the kernel's memory-pressure figures "
              "rising, 1920x1080 ran anywhere from 322 to 463 ms: if that row is much slower "
              "for you, look at swap before anything else.")


def clamp(knob, value):
    """A value the daemon will accept, or None if this knob does not take numbers."""
    if knob.kind != "number":
        return None
    stepped = round(value / knob.step) * knob.step
    return round(min(max(stepped, knob.low), knob.high), 4)
