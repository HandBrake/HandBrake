#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define BUF_SIZE    256

void
usage(char *cmd)
{
    printf("%s\n", cmd);
    char *base = basename(cmd);
    fprintf(stderr, "Usage: %s infile [outfile]\n", base);
}

int
main(int argc, char *argv[])
{
    FILE *infile, *outfile;
    char buffer[BUF_SIZE];

    if (argc < 2 || argc > 3)
    {
        usage(argv[0]);
        return 1;
    }
    infile = fopen(argv[1], "r");
    if (argc < 3)
    {
        outfile = stdout;
    }
    else
    {
        outfile = fopen(argv[2], "w");
    }
    while (fgets(buffer, BUF_SIZE, infile) != NULL)
    {
        int len;
        // Step on any CR LF at end of line
        len = strlen(buffer);
        if (buffer[len-1] == '\n' || buffer[len-1] == '\r')
            buffer[len-1] = 0;
        if (buffer[len-2] == '\n' || buffer[len-2] == '\r')
            buffer[len-2] = 0;
        fprintf(outfile, "\"%s\\n\"\n", buffer);
    }
    close(infile);
    close(outfile);
}
