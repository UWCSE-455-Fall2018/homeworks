#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "matrix.h"
#include "image.h"
#include "test.h"
#include "args.h"

int within_eps(float a, float b){
    return a-EPS<b && b<a+EPS;
}

int same_image(image a, image b){
    int i;
    if(a.w != b.w || a.h != b.h || a.c != b.c) {
        printf("Expected %d x %d x %d image, got %d x %d x %d\n", b.w, b.h, b.c, a.w, a.h, a.c);
        return 0;
    }
    for(i = 0; i < a.w*a.h*a.c; ++i){
        if(!within_eps(a.data[i], b.data[i])) 
        {
            printf("The value should be %f, but it is %f! \n", b.data[i], a.data[i]);
            return 0;
        }
    }
    return 1;
}

void run_tests()
{
    printf("no tests required");
}