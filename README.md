# argz.h

A single-file command-line parser library.

## Quick start

This example shows how to use **argz.h**:

```c
#include <stdio.h>

#define ARGZ_IMPLEMENTATION
#include "argz.h"

void copy(const char *input, const char *output, long size);

int main(int argc, const char *argv[]) {

    const char *input  = NULL;
    const char *output = "output.txt";
    long size          = 128;
    int help           = 0;

    argz_str("-i",     "Input file path.",                        &input);
    argz_str("-o",     "Output file path (default: output.txt).", &output);
    argz_lng("--size", "Buffer size in bytes.",                   &size);
    argz_flg("-h",     "Print this message and exit.",            &help);

    argz_parse(argc, argv);

    if (help) {
        argz_options_print();
        return 0;
    }

    copy(input, output, size);
}

void copy(const char *input, const char *output, long size) {
    if (!input) {
        printf("Error: No input\n");
        return;
    }
    printf("copy: %s => [%ld] => %s\n", input, size, output);
}
```

Compile & run the example:

```console
$ ls
argz.h main.c
$ cc main.c -o copy
$ ./copy -h
Options:
  -i       Input file path.
  -o       Output file path (default: output.txt).
  --size   Buffer size in bytes.
  -h       Print this message and exit.
$ ./copy
Error: No input
$ ./copy -i input.txt
copy: input.txt => [128] => output.txt
$ ./copy -i input.txt --size 1024 -o other.txt
copy: input.txt => [1024] => other.txt
```

No allocation needed. Pointers are stored in static memory.

## Customize

- **ARGZ_COUNT**

Changes the maximum number of parameters allowed (default = 8).

```c
#define ARGZ_COUNT 2
#define ARGZ_IMPLEMENTATION
#include "argz.h"

int main(int argc, const char *argv[])
{
    // I only need 2 argz !
    long arg_1 = 1;
    long arg_2 = 2;
    argz_lng("--one", "Argument one.", &arg_1);
    argz_lng("--two", "Argument two.", &arg_2);

    // Changed my mind, gimme one more !
    int arg_3 = 0;
    argz_flg("-t", "One more please !", &arg_3);
}
```

Will print:

```console
$ ./program --one 1 --two 2
ERROR: ARGZ_COUNT=2 exceeded, for option '-t'
```

- **ARGZ_STATIC**

Declares & defines all **argz\_** functions 'static'.

```c
#define ARGZ_STATIC
#include "argz.h"

// Turns:
void argz_parse(int argc, const char *argv[]);
void argz_parse(int argc, const char *argv[]) {
    ...
}

// Into:
static void argz_parse(int argc, const char *argv[]);
static void argz_parse(int argc, const char *argv[]) {
    ...
}
```
