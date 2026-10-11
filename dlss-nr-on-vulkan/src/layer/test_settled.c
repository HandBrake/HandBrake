/* The interface detector: which pixels held still, and when to refuse to guess.
 *
 * The daemon side is proved exact by test_ui_mask.py. This is the other half — the
 * decision the layer makes before anything is sent, which no test reached before.
 */
#include "nr_layer.c"

static int failures;

static void check(const char *name, int ok, const char *detail)
{
	printf("  [%s] %s%s%s\n", ok ? "ok  " : "FAIL", name, detail[0] ? "  " : "", detail);
	if (!ok) failures++;
}

/* A frame that is a scene on the left and an unchanging panel on the right. */
static void build(unsigned char *now, unsigned char *before, uint32_t width,
		  uint32_t height, uint32_t panel_from, int scene_step, int panel_step)
{
	for (uint32_t y = 0; y < height; y++)
		for (uint32_t x = 0; x < width; x++) {
			uint32_t i = 4 * (y * width + x);
			int base = (int)((x * 7 + y * 13) % 200) + 20;
			int step = x >= panel_from ? panel_step : scene_step;
			for (int c = 0; c < 3; c++) {
				before[i + c] = (unsigned char)base;
				now[i + c] = (unsigned char)(base + step);
			}
			before[i + 3] = now[i + 3] = 255;
		}
}

int main(void)
{
	const uint32_t width = 320, height = 180, pixels = width * height;
	unsigned char *now = malloc(4 * pixels), *before = malloc(4 * pixels);
	unsigned char *mask = malloc(pixels);
	char detail[160];
	if (!now || !before || !mask) return 2;

	/* A quarter of the frame is a panel that does not move, the rest is a scene. */
	uint32_t panel_from = width * 3 / 4;
	build(now, before, width, height, panel_from, 40, 0);
	uint32_t held = settled(now, before, pixels, mask);
	uint32_t expect = (width - panel_from) * height;
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(detail, sizeof detail, "%u pixels held, expected %u", held, expect);
#else
	snprintf(detail, sizeof detail, "%u pixels held, expected %u", held, expect);
#endif

	check("a still panel over a moving scene is found exactly", held == expect, detail);

	int marked_panel = 1, marked_scene = 0;
	for (uint32_t y = 0; y < height; y++)
		for (uint32_t x = 0; x < width; x++) {
			if (x >= panel_from && !mask[y * width + x]) marked_panel = 0;
			if (x < panel_from && mask[y * width + x]) marked_scene = 1;
		}
	check("every panel pixel is marked and no scene pixel is", marked_panel && !marked_scene, "");
	check("and that mask is worth sending", mask_worth_sending(held, pixels), "");

	/* Dither: a still panel in a real game is not bit-identical between presents. */
	build(now, before, width, height, panel_from, 40, 2);
	held = settled(now, before, pixels, mask);
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(detail, sizeof detail, "two levels per channel still counts as held (%u)", held);
#else
	snprintf(detail, sizeof detail, "two levels per channel still counts as held (%u)", held);
#endif

	check("small dither does not break the panel", held == expect, detail);

	build(now, before, width, height, panel_from, 40, 3);
	held = settled(now, before, pixels, mask);
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(detail, sizeof detail, "three levels per channel counts as motion (%u held)", held);
#else
	snprintf(detail, sizeof detail, "three levels per channel counts as motion (%u held)", held);
#endif

	check("a real change is not swallowed by the tolerance", held == 0, detail);

	/* Nothing moved: a paused scene is indistinguishable from an interface. */
	build(now, before, width, height, panel_from, 0, 0);
	held = settled(now, before, pixels, mask);
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(detail, sizeof detail, "%u of %u held", held, pixels);
#else
	snprintf(detail, sizeof detail, "%u of %u held", held, pixels);
#endif

	check("a frozen frame is refused rather than guessed",
	      held == pixels && !mask_worth_sending(held, pixels), detail);

	/* Everything moved: there is no interface to protect. */
	build(now, before, width, height, width, 40, 40);
	held = settled(now, before, pixels, mask);
	check("a frame with no still pixels sends no mask",
	      held == 0 && !mask_worth_sending(held, pixels), "");

	/* A thin HUD — a health bar — is above the floor and still worth sending. */
	build(now, before, width, height, width - width / 20, 40, 0);
	held = settled(now, before, pixels, mask);
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(detail, sizeof detail, "%u%% of the frame", 100 * held / pixels);
#else
	snprintf(detail, sizeof detail, "%u%% of the frame", 100 * held / pixels);
#endif

	check("a thin HUD is above the floor", mask_worth_sending(held, pixels), detail);

	free(now); free(before); free(mask);
	printf("\n%s\n", failures ? "the interface detector is wrong" : "the interface detector behaves");
	return failures ? 1 : 0;
}
