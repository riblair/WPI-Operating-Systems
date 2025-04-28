// DO NOT MODIFY THIS SERIAL VERSION 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Setup variables.
uint32_t count = 0;
char current;
char c;

void rle(char* fname){
    // Open the file, if it does not exist, exit with error.
    FILE *stream = fopen(fname, "r");
    if (stream == NULL) {
        printf("wzip: cannot open file\n");
        exit(1);
    }

    // Go through each char, and count how many times it repeats.
    // If the char changes, output the previous count and char, and start the next section.

    // First character setup
    c = fgetc(stream);
    if (!feof(stream) && count == 0) {
        current = c;
        count++;
        c = fgetc(stream);
    }

    // Loop through each other character.
    while (!feof(stream)) {
        if (c != current) {
            fwrite(&count, 4, 1, stdout);
            fwrite(&current, 1, 1, stdout);
            current = c;
            count = 1;
        } else {
            count++;
        }
        c = fgetc(stream);
    }

    // Close the stream.
    fclose(stream);
}

int main(int argc, char **argv) {
    // If there aren't enough arguments, exit with error.
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }


    // Go through each file.
    for (int i = 1; i < argc; i++) {
        rle(argv[i]);
    }

    // Write the last character.
    fwrite(&count, 4, 1, stdout);
    fwrite(&current, 1, 1, stdout);

    // Exit with success.
    exit(0);
}