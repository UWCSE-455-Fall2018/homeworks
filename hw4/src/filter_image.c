#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "image.h"
#define TWOPI 6.2831853

image add_image(image a, image b)
{
    assert(a.w == b.w && a.h == b.h && a.c == b.c);
    int i;
    image sum = make_image(a.w, a.h, a.c);
    for(i = 0; i < a.w*a.h*a.c; ++i){
        sum.data[i] = a.data[i] + b.data[i];
    }
    return sum;
}

image sub_image(image a, image b)
{
    assert(a.w == b.w && a.h == b.h && a.c == b.c);
    int i;
    image sum = make_image(a.w, a.h, a.c);
    for(i = 0; i < a.w*a.h*a.c; ++i){
        sum.data[i] = a.data[i] - b.data[i];
    }
    return sum;
}

void print_image(image im)
{
    int i,j,k;
    for(k = 0; k < im.c; ++k){
        printf("\nChannel %d\n", k);
        for(j = 0; j < im.h; ++j){
            for(i = 0; i < im.w; ++i){
                printf("%5.3f ", im.data[i+im.w*(j + im.h*k)]);
            }
            printf("\n");
        }
    }
}