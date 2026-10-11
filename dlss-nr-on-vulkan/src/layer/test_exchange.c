/* Exercise the game's actual socket exchange with SIGPIPE's default disposition.
 *
 * `masked` is the case that matters most and was missed: with an interface mask the
 * request carries a byte plane the answer does not, so the send and the receive have
 * different sizes. Driving the daemon through its own socket from Python does not
 * exercise this — only `exchange()` does — and while that was untested every masked
 * frame in a real game came back unchanged. */
#include "nr_layer.c"
#include <signal.h>

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    signal(SIGPIPE, SIG_DFL);
    socket_path = argv[1];
    int reject = strcmp(argv[2], "reject") == 0;
    int masked = strcmp(argv[2], "masked") == 0;
    unsigned width = reject ? 2048 : 64, height = reject ? 1024 : 32;
    size_t bytes = (size_t)width * height * 4;
    size_t pixels = (size_t)width * height;
    size_t sent = masked ? bytes + pixels : bytes;
    unsigned char *payload = calloc(sent, 1), *reply = malloc(bytes);
    if (!payload || !reply) return 2;
    /* the reply buffer is exactly the colour, as the layer's is: asking for more than
     * that is what the separate reply size exists to prevent */
    uint32_t header[] = { masked ? 0x314E524E : 0x304E524E, width, height, 44 };
    int result = exchange(header, sizeof header, payload, sent, reply, bytes);
    int ok = reject ? result == -1
                    : result == 0 && memcmp(payload, reply, bytes) == 0;
    free(payload); free(reply);
    return ok ? 0 : 1;
}
