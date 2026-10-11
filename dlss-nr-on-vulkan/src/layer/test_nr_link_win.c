/* test_nr_link_win.c — verify the layer's transport header talks to the Python
 * nr_pipe server. Builds only on Windows; mirrors the A.7.10 round trip. */
#include <stdio.h>
#include "nr_transport.h"

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "\\\\.\\pipe\\nr_link_test";
    nr_link link;
    if (nr_link_connect(&link, path) != 0) {
        printf("connect failed\n");
        return 1;
    }
    printf("connected\n");
    const char *msg = "HELLO-FROM-C-LINK";
    if (nr_link_write(&link, msg, 17) != 0) {
        printf("write failed\n");
        nr_link_close(&link);
        return 2;
    }
    printf("wrote 17\n");
    unsigned char reply[32] = {0};
    if (nr_link_read(&link, reply, 16) != 0) {
        printf("read failed\n");
        nr_link_close(&link);
        return 3;
    }
    printf("read: %s\n", reply);
    nr_link_close(&link);
    return 0;
}
