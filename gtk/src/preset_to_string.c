#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define BUF_SIZE    4*65536

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
    char in_buffer[BUF_SIZE];
    char out_buffer[2*BUF_SIZE];

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
    while (fgets(in_buffer, BUF_SIZE, infile) != NULL)
    {
		int ii, jj;
        int len;
        // Step on any CR LF at end of line
        len = strlen(in_buffer);
		for (jj = 0, ii = 0; ii < len; ii++)
		{
			if (in_buffer[ii] == '"')
			{
				out_buffer[jj++] = '\\';
				out_buffer[jj++] = in_buffer[ii];
			}
        	else if (in_buffer[ii] == '\n' || in_buffer[ii] == '\r')
			{ // Skip it
			}
			else
			{
				out_buffer[jj++] = in_buffer[ii];
			}
		}
		out_buffer[jj] = 0;
        fprintf(outfile, "\"%s\\n\"\n", out_buffer);
    }
    fclose(infile);
    fclose(outfile);
	return 0;
}
