// argz.h
// Utterly simple C command-line arguments parser
//
// This file provides both the interface and the implementation.
// To instantiate the implementation:
//      #define ARGZ_IMPLEMENTATION
// in *ONE* source file, before #including this file.
//
// Licence
//     See end of file.

#ifndef INCLUDE_ARGZ_H
#define INCLUDE_ARGZ_H

#ifdef ARGZ_STATIC
#define ARGZ_CDEC static
#else
#define ARGZ_CDEC extern
#endif // ARGZ_STATIC

// API

// Add an argument of type 'double'.
// {
//     double arg = 0;
//     argz_dbl("-d", "A double argument.", &arg);
// }
ARGZ_CDEC void argz_dbl(const char *option, const char *desc, double *addr);

// Add an argument of type 'long'.
// {
//     long arg = 0;
//     argz_lng("-l", "A long argument.", &arg);
// }
ARGZ_CDEC void argz_lng(const char *option, const char *desc, long *addr);

// Add a flag argument (int).
// {
//     int help = 0;
//     argz_flg("-h", "Print help.", &help);
// }
ARGZ_CDEC void argz_flg(const char *option, const char *desc, int *addr);

// Add a string argument of (const char *).
// {
//     const char *name = 0;
//     argz_str("--name", "Say my name.", &name);
// }
ARGZ_CDEC void argz_str(const char *option, const char *desc, const char **addr);

// Parse arguments.
// {
//     long size = 0;
//     argz_lng("--size", "It doesn't matter.", &size);
//
//     int argc = 3;
//     const char *argv[] = { "/bin/prog", "--size", "10" };
//
//     argz_parse(argc, argv);
//
//     assert(size == 10);
// }
ARGZ_CDEC void argz_parse(int argc, const char *argv[]);

// Print options.
ARGZ_CDEC void argz_options_print(void);

#undef ARGZ_CDEC

#endif // INCLUDE_ARGZ_H

// Example
#if 0
#include <stdio.h>

#define ARGZ_IMPLEMENTATION
#include "argz.h"

void copy(const char *input, const char *output, long size);

int main(int argc, const char *argv[])
{
    const char *input = NULL;
    const char *output = "output.txt";
    long size = 128;
    int help = 0;

    argz_str("-i", "Input file path.", &input);
    argz_str("-o", "Output file path (default: output.txt).", &output);
    argz_lng("--size", "Buffer size in bytes.", &size);
    argz_flg("-h", "Print this message and exit.", &help);

    argz_parse(argc, argv);

    if (help)
    {
        argz_options_print();
        return 0;
    }

    copy(input, output, size);
}

void copy(const char *input, const char *output, long size)
{
    if (!input)
    {
        printf("Error: No input\n");
        return;
    }
    printf("copy: %s => [%ld] => %s\n", input, size, output);
}
#endif

#ifdef ARGZ_IMPLEMENTATION

#include <stdio.h>  // printf, fprintf
#include <stdlib.h> // abort, exit, strtod, strtol
#include <string.h> // strcmp, strlen

#ifdef ARGZ_STATIC
#define ARGZ_CDEF static
#else
#define ARGZ_CDEF
#endif // ARGZ_STATIC

#ifndef ARGZ_COUNT
#define ARGZ_COUNT 8
#endif // ARGZ_COUNT

#define _ARGZ_DBL 0
#define _ARGZ_FLG 1
#define _ARGZ_LNG 2
#define _ARGZ_STR 3

static const char *_argz_option_at(size_t pos, const char *option);
static const char *_argz_desc_at(size_t pos, const char *desc);
static void *_argz_value_addr_at(size_t pos, void *addr);
static int _argz_type_at(size_t pos, int type);
static void _argz_init_next(const char *option, const char *desc, void *addr, int type);
static void _argz_parse_arg(void *value_addr, int type, const char *option, const char *argv);
static size_t _argz_count(size_t inc);

ARGZ_CDEF void argz_dbl(const char *option, const char *desc, double *addr)
{
    _argz_init_next(option, desc, addr, _ARGZ_DBL);
}

ARGZ_CDEF void argz_flg(const char *option, const char *desc, int *addr)
{
    _argz_init_next(option, desc, addr, _ARGZ_FLG);
}

ARGZ_CDEF void argz_lng(const char *option, const char *desc, long *addr)
{
    _argz_init_next(option, desc, addr, _ARGZ_LNG);
}

ARGZ_CDEF void argz_str(const char *option, const char *desc, const char **addr)
{
    _argz_init_next(option, desc, addr, _ARGZ_STR);
}

ARGZ_CDEF void argz_parse(int argc, const char *argv[])
{
    const size_t argz_count = _argz_count(0);
    // ignore argv[0]
    for (int c = 1; c < argc; c++)
    {
        const char *v = argv[c];
        if (v == NULL)
        {
            fprintf(stderr, "ERROR: argv[%d] is null\n", c);
            exit(1);
        }
        for (size_t z = 0; z < argz_count; z++)
        {
            const char *option = _argz_option_at(z, NULL);
            const int type = _argz_type_at(z, -1);
            void *value_addr = _argz_value_addr_at(z, NULL);
            if (option == NULL || value_addr == NULL)
            {
                abort();
            }
            if (strcmp(v, option))
            {
                // argv != option
                continue;
            }
            if (type == _ARGZ_FLG)
            {
                *((int *)value_addr) = 1;
                break;
            }
            c += 1;
            if (c >= argc)
            {
                return;
            }
            _argz_parse_arg(value_addr, type, option, argv[c]);
        }
    }
}

ARGZ_CDEF void argz_options_print(void)
{
    const size_t argz_count = _argz_count(0);
    size_t opt_max_len = 0;
    for (size_t i = 0; i < argz_count; i++)
    {
        const char *option = _argz_option_at(i, NULL);
        size_t len = 0;
        if (option == NULL)
        {
            abort();
        }
        len = strlen(option);
        if (len > opt_max_len)
        {
            opt_max_len = len;
        }
    }
    printf("Options:\n");
    for (size_t i = 0; i < argz_count; i++)
    {
        const char *option = _argz_option_at(i, NULL);
        const char *desc = _argz_desc_at(i, NULL);
        int spaces = 0;
        if (option == NULL)
        {
            abort();
        }
        spaces = (int)(opt_max_len - strlen(option) + 1);
        printf("  %s %*s %s\n", option, spaces, "", (desc != NULL) ? desc : "?");
    }
    printf("\n");
}

static const char *_argz_option_at(size_t pos, const char *option)
{
    static const char *options[ARGZ_COUNT] = {0};
    if (pos >= ARGZ_COUNT)
    {
        return NULL;
    }
    if (option != NULL)
    {
        options[pos] = option;
    }
    return options[pos];
}

static const char *_argz_desc_at(size_t pos, const char *desc)
{
    static const char *descs[ARGZ_COUNT] = {0};
    if (pos >= ARGZ_COUNT)
    {
        return NULL;
    }
    if (desc != NULL)
    {
        descs[pos] = desc;
    }
    return descs[pos];
}

static void *_argz_value_addr_at(size_t pos, void *value_addr)
{
    static void *value_addrs[ARGZ_COUNT] = {0};
    if (pos >= ARGZ_COUNT)
    {
        return NULL;
    }
    if (value_addr != NULL)
    {
        value_addrs[pos] = value_addr;
    }
    return value_addrs[pos];
}

static int _argz_type_at(size_t pos, int type)
{
    static int types[ARGZ_COUNT] = {0};
    if (pos >= ARGZ_COUNT)
    {
        return -1;
    }
    if (type != -1)
    {
        types[pos] = type;
    }
    return types[pos];
}

static void _argz_init_next(const char *option, const char *desc, void *addr, int type)
{
    const size_t count = _argz_count(0);
    const char *opt = NULL;
    if (option == NULL)
    {
        fprintf(stderr, "ERROR: option cannot be null\n");
        exit(1);
    }
    if (addr == NULL)
    {
        fprintf(stderr, "ERROR: value address cannot be null\n");
        exit(1);
    }
    if (strlen(option) == 0)
    {
        fprintf(stderr, "ERROR: option cannot be empty\n");
        exit(1);
    }
    opt = _argz_option_at(count, option);
    if (opt == NULL)
    {
        fprintf(stderr, "ERROR: ARGZ_COUNT=%lu exceeded, for option '%s'\n", count, option);
        exit(1);
    }
    // save
    (void)_argz_desc_at(count, desc);
    (void)_argz_value_addr_at(count, addr);
    (void)_argz_type_at(count, type);
    // increment arguments count
    (void)_argz_count(1);
}

static void _argz_parse_arg(void *value_addr, int type, const char *option, const char *argv)
{
    switch (type)
    {
    case _ARGZ_DBL: {
        char *endptr = NULL;
        *((double *)value_addr) = strtod(argv, &endptr);
        if (endptr == argv)
        {
            fprintf(stderr, "ERROR: Failed to parse option '%s'. Expected %s, got '%s'.\n", option, "double", argv);
            exit(1);
        }
    }
    break;
    case _ARGZ_LNG: {
        char *endptr = NULL;
        *((long *)value_addr) = strtol(argv, &endptr, 10);
        if (endptr == argv)
        {
            fprintf(stderr, "ERROR: Failed to parse option '%s'. Expected %s, got '%s'.\n", option, "long", argv);
            exit(1);
        }
    }
    break;
    case _ARGZ_STR: {
        *((const char **)value_addr) = argv;
    }
    break;
    default:
        fprintf(stderr, "ERROR: type '%d' not supported\n", type);
        exit(1);
    }
}

static size_t _argz_count(size_t inc)
{
    static size_t count = 0;
    if (count >= ARGZ_COUNT)
    {
        abort();
    }
    count += inc;
    return count;
}

#undef ARGZ_CDEF
#undef _ARGZ_DBL
#undef _ARGZ_FLG
#undef _ARGZ_LNG
#undef _ARGZ_STR

#endif // ARGZ_IMPLEMENTATION

/*
MIT License

Copyright (c) 2024 Réda Frauly

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
