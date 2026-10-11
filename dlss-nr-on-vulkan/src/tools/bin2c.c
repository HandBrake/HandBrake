/*
BSD 2-Clause License

Copyright (c) 2016, Rafael Kitover
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <locale.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <wctype.h>

#define BUF_SIZE  4096
#define WBUF_SIZE BUF_SIZE * sizeof(wchar_t)
#define MSG_SIZE  256

const char* version    = "0.3";

const char* msg_prefix = "bin2c: ";

void format_perror(const char* fmt, va_list args) {
    static char error_str[MSG_SIZE];
    static char fmt_str[MSG_SIZE];
#if __STDC_WANT_SECURE_LIB__
    strcpy_s(fmt_str, sizeof(fmt_str), msg_prefix);
    strncat_s(fmt_str, sizeof(fmt_str), fmt, MSG_SIZE - strlen(msg_prefix));
    vsprintf_s(error_str, MSG_SIZE, fmt_str, args);
#else
    strcpy(fmt_str, msg_prefix);
    strncat(fmt_str, fmt, MSG_SIZE - strlen(msg_prefix));
    vsnprintf(error_str, MSG_SIZE, fmt_str, args);
#endif
    perror(error_str);
}

void die(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    format_perror(fmt, args);
    va_end(args);
    exit(1);
}

void* safe_malloc(size_t size) {
    void* allocated = malloc(size);
    if (!allocated) {
        die("out of memory");
    }
    return allocated;
}

char* file_name_to_identifier(const char* file_name_cstr) {
    wchar_t* file_name       = safe_malloc(WBUF_SIZE);
    wchar_t* identifier      = safe_malloc(WBUF_SIZE);
    char*    identifier_cstr = safe_malloc(WBUF_SIZE);
    wchar_t* file_name_ptr   = file_name;
    wchar_t* identifier_ptr  = identifier;
    wchar_t* file_name_end   = NULL;
    size_t   file_name_len   = 0;
    int between_tokens       = 0;

#if __STDC_WANT_SECURE_LIB__
    mbstowcs_s(&file_name_len, file_name, WBUF_SIZE, file_name_cstr, BUF_SIZE - 1);
#else
    file_name_len = mbstowcs(file_name, file_name_cstr, BUF_SIZE - 1);
#endif
    if (file_name_len == (size_t)(-1)) {
        die("cannot convert '%s' to locale representation", file_name_cstr);
    }

    *identifier = 0;

    file_name_end = file_name + file_name_len + 1;

    while (file_name_ptr < file_name_end) {
        if (iswalnum(*file_name_ptr)) {
            *identifier_ptr = *file_name_ptr;
            identifier_ptr++;
            between_tokens = 0;
        }
        else if (!between_tokens) {
#if __STDC_WANT_SECURE_LIB__
            size_t identifier_ptr_sz = 0;
            mbstowcs_s(&identifier_ptr_sz, identifier_ptr, WBUF_SIZE, "_", 1);
#else
            mbstowcs(identifier_ptr, "_", 1);
#endif
            identifier_ptr++;
            between_tokens  = 1;
        }

        file_name_ptr++;
    }

    /* terminate identifier, on _ or on next position */
    if (between_tokens) identifier_ptr--;

    *identifier_ptr = 0;

#if __STDC_WANT_SECURE_LIB__
    size_t identifier_cstr_sz = 0;
    wcstombs_s(&identifier_cstr_sz, identifier_cstr, BUF_SIZE, identifier, WBUF_SIZE - 1);
    if (identifier_cstr_sz == (size_t)(-1))
#else
    if (wcstombs(identifier_cstr, identifier, WBUF_SIZE - 1) == (size_t)(-1))
#endif
        die("failed to convert wide character string to bytes");

    free(file_name);
    free(identifier);

    return identifier_cstr;
}

void usage(int err) {
    FILE* stream = err ? stderr : stdout;

    fputs(
"Usage: [32mbin2c [1;34m[[1;35mIN_FILE [1;34m[[1;35mOUT_FILE [1;34m[[1;35mVAR_NAME[1;34m][1;34m][1;34m][0m\n"
"Write [1;35mIN_FILE[0m as a C array of bytes named [1;35mVAR_NAME[0m into [1;35mOUT_FILE[0m.\n"
"\n"
"By default, [1mSTDIN[0m is the input and [1mSTDOUT[0m is the output, either can be explicitly specified with [1;35m-[0m.\n"
"\n"
"The default [1;35mVAR_NAME[0m is the [1;35mIN_FILE[0m name converted to an identifier, or [1m\"resource_data\"[0m\n"
"if it's [1mSTDIN[0m.\n"
"\n"
"  [1m-h, --help[0m                        Show this help screen and exit.\n"
"  [1m-v, --version[0m                     Print version and exit.\n"
"\n"
"Examples:\n"
"  # write data from file [1;35m./compiled-resources.xrs[0m into header file [1;35m./resources.h[0m using variable name [1;35mresource_data[0m\n"
"  [32mbin2c [1;35m./compiled-resources.xrs[0m [1;35m./resources.h[0m [1;35mresource_data[0m\n"
"  # write data from [1mSTDIN[0m to [1mSTDOUT[0m with [1m\"resource_data\"[0m as the [1;35mVAR_NAME[0m\n"
"  [32mbin2c[0m\n"
"  # write data from [1;35m./compiled-resources.xrs[0m to [1mSTDOUT[0m with [1m\"compiled_resources_xrs\"[0m as the [1;35mVAR_NAME[0m\n"
"  [32mbin2c [1;35m./compiled-resources.xrs[0m\n"
"  # write data from [1;35m./compiled-resources.xrs[0m to [1;35m./resources.h[0m with [1m\"compiled_resources_xrs\"[0m as the [1;35mVAR_NAME[0m\n"
"  [32mbin2c [1;35m./compiled-resources.xrs [1;35m./resources.h[0m\n"
"\n"
"Project homepage and documentation: <[1;34mhttp://github.com/rkitover/bin2c[0m>\n"
    , stream);
}

void die_usage(const char* fmt, ...) {
    static char fmt_str[MSG_SIZE];
    va_list args;
    va_start(args, fmt);
#if __STDC_WANT_SECURE_LIB__
    strcpy_s(fmt_str, sizeof(fmt_str), msg_prefix);
#else
    strcpy(fmt_str, msg_prefix);
#endif
    // Need to reserve 1 byte for the newline.
#if __STDC_WANT_SECURE_LIB__
    strncat_s(fmt_str, sizeof(fmt_str), fmt, MSG_SIZE - strlen(msg_prefix) - 1);
    strcat_s(fmt_str, sizeof(fmt_str), "\n");
#else
    strncat(fmt_str, fmt, MSG_SIZE - strlen(msg_prefix) - 1);
    strcat(fmt_str, "\n");
#endif
    vfprintf(stderr, fmt_str, args);
    va_end(args);
    usage(1);
    exit(1);
}

void exit_usage(int exit_code) {
    usage(exit_code);
    exit(exit_code);
}

int main(int argc, const char** argv) {
    const char* usage_opts[]   = {"-h", "--help"};
    const char* version_opts[] = {"-v", "--version"};
    const char* in_file_name   = argc >= 2 ? argv[1] : NULL;
    const char* out_file_name  = argc >= 3 ? argv[2] : NULL;
    const char* var_name       = argc >= 4 ? argv[3] : NULL;
    char* computed_identifier  = NULL;
    size_t i = 0;
    int file_pos = 0;
    size_t bytes_read   = 0;
    unsigned char* buf  = safe_malloc(BUF_SIZE);
    FILE *in_file, *out_file;

    setlocale(LC_ALL, "");

    if (argc > 4)
        die_usage("invalid number of arguments");

    if (argc >= 2) {
        for (i = 0; i < (sizeof(usage_opts)/sizeof(char*)); i++) {
            if (!strcmp(argv[1], usage_opts[i])) exit_usage(0);
        }

        for (i = 0; i < (sizeof(version_opts)/sizeof(char*)); i++) {
            if (!strcmp(argv[1], version_opts[i])) {
                printf("bin2c %s\n", version);
                return 0;
            }
        }
    }

    if (!in_file_name || !strcmp(in_file_name, "-")) {
        in_file = stdin;
    } else {
#if __STDC_WANT_SECURE_LIB__
        fopen_s(&in_file, in_file_name, "rb");
#else
        in_file = fopen(in_file_name, "rb");
#endif
        if (!in_file) {
            die("can't open input file '%s'", in_file_name);
        }
    }

    if (!out_file_name || !strcmp(out_file_name, "-")) {
        out_file = stdout;
    } else {
#if __STDC_WANT_SECURE_LIB__
        fopen_s(&out_file, out_file_name, "w");
#else
        out_file = fopen(out_file_name, "w");
#endif
        if (!out_file) {
            die("can't open output file '%s'", out_file_name);
        }
    }

    if (in_file_name && !var_name)
        var_name = computed_identifier = file_name_to_identifier(in_file_name);

    fprintf(out_file, "/* generated from %s: do not edit */\n"
                      "const unsigned char %s[] = {\n",
                      in_file_name ? in_file_name : "resource data",
                      var_name     ? var_name     : "resource_data"
    );

    bytes_read = fread(buf, 1, BUF_SIZE, in_file);
    while (bytes_read != 0) {
        for (i = 0; i < bytes_read; i++) {
            char* comma = bytes_read < BUF_SIZE && i == bytes_read - 1 ? "" : ",";

            fprintf(out_file, "0x%02x%s", buf[i], comma);

            file_pos++;
            if (file_pos % 16 == 0) {
                fputc('\n', out_file);
            }
        }
        bytes_read = fread(buf, 1, BUF_SIZE, in_file);
    }

    fputs("};\n", out_file);

    free(buf);
    free(computed_identifier);
    fclose(in_file);
    fclose(out_file);

    return 0;
}
