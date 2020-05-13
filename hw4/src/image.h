#ifndef IMAGE_H
#define IMAGE_H
#include <stdio.h>

#include "matrix.h"
#define TWOPI 6.2831853

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// DO NOT CHANGE THIS FILE

typedef struct{
    int w,h,c;
    float *data;
} image;

// A 2d point.
// float x, y: the coordinates of the point.
typedef struct{
    float x, y;
} point;

// A descriptor for a point in an image.
// point p: x,y coordinates of the image pixel.
// int n: the number of floating point values in the descriptor.
// float *data: the descriptor for the pixel.
typedef struct{
    point p;
    int n;
    float *data;
} descriptor;

// A match between two points in an image.
// point p, q: x,y coordinates of the two matching pixels.
// int ai, bi: indexes in the descriptor array. For eliminating duplicates.
// float distance: the distance between the descriptors for the points.
typedef struct{
    point p, q;
    int ai, bi;
    float distance;
} match;

// Basic operations
image sub_image(image a, image b);
image add_image(image a, image b);

// Loading and saving
image make_image(int w, int h, int c);
image load_image(char *filename);
void save_image(image im, const char *name);
void save_png(image im, const char *name);
void free_image(image im);

#ifndef __cplusplus
    #ifdef OPENCV
        #include "opencv2/highgui/highgui_c.h"
        #include "opencv2/imgproc/imgproc_c.h"
        #include "opencv2/core/version.hpp"
        #if CV_MAJOR_VERSION == 3
            #include "opencv2/videoio/videoio_c.h"
            #include "opencv2/imgcodecs/imgcodecs_c.h"
        #endif
        image get_image_from_stream(CvCapture *cap);
        int show_image(image im, const char *name, int ms);
    #endif
#endif

// Machine Learning

typedef enum{LINEAR, LOGISTIC, TANH, RELU, LRELU, SOFTMAX} ACTIVATION;

typedef struct {
    matrix in;              // Saved input to a layer
    matrix w;               // Current weights for a layer
    matrix dw;              // Current weight updates
    matrix v;               // Past weight updates (for use with momentum)
    matrix out;             // Saved output from the layer
    ACTIVATION activation;  // Activation the layer uses
} layer;

typedef struct{
    matrix X;
    matrix y;
} data;

typedef struct {
    layer *layers;
    int n;
} model;

data load_classification_data(char *images, char *label_file, int bias);
void free_data(data d);
data random_batch(data d, int n);
char *fgetl(FILE *fp);

#endif

