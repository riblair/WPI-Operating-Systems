// here is the skeleton code;
// to use, copy everything to the "goatfs.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "disk.h"
#include "goatfs.h"


void debug(){

}


bool format(){

    return true;
}

int mount(){
    return -1;
}

ssize_t create(){
    return 0;
}
bool wremove(size_t inumber){
    return true;
}
ssize_t stat(size_t inumber) {
    return 0;
}
ssize_t wfsread(size_t inumber, char* data, size_t length, size_t offset){
    return 0;
}
ssize_t wfswrite(size_t inumber, char* data, size_t length, size_t offset){
    return 0;
}