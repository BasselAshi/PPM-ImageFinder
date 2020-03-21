#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <float.h>
#include "worker.c"

int main(int argc, char **argv)
{
    char *hi = "testing/1/pyramid.ppm";
    // char *yo = "none.ppm";
    Image *img = read_image(hi);
    // float hello = compare_images(img, yo);
    // printf("here: %f\n", hello);

    CompRecord cmp = process_dir(".", img, STDOUT_FILENO);
    printf("File name: %s\t\tDistance: %f\n", cmp.filename, cmp.distance);

    // float currentValue = compare_images(img, "pyramid.ppm");
    // printf("here: %f\n", currentValue);
}