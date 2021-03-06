#pragma once

#include <cassert>
#include <cstring>
#include <cmath>

#include <algorithm>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

#include "matrix.h"
#include "utils.h"


// DO NOT CHANGE THIS FILE


struct Image
  {
  int w,h,c;
  float*data;
  
  // constructor
  Image() : Image(0,0,0) {}
  Image(int w, int h, int c=1) : w(w), h(h), c(c), data(nullptr) { if(w*h*c)data=(float*)calloc(w*h*c,sizeof(float)); }
  
  // destructor
  ~Image() { free(data); }
  
  // copy constructor
  Image(const Image& a) : data(nullptr) { *this=a; }
  
  // move constructor
  Image(Image&& a)  : data(nullptr) { *this=move(a); }
  
  // copy assignment
  Image& operator=(const Image& a)
    {
    if(this==&a)return *this;
    
    if(data){free(data);data=nullptr;}
    w=a.w;
    h=a.h;
    c=a.c;
    data=(float*)calloc(w*h*c,sizeof(float));
    memcpy(data,a.data,sizeof(float)*w*h*c);
    return *this;
    }
  
  // move assignment
  Image& operator=(Image&& a)
    {
    if(this==&a)return *this;
    
    if(data)free(data);
    
    w=a.w;
    h=a.h;
    c=a.c;
    data=a.data;
    
    a.data=nullptr;
    a.w=a.h=a.c=0;
    
    return *this;
    }
  
  // pixel access
  
  float& operator()(int x, int y, int ch)
    {
    assert(ch<c && ch>=0 && x<w && x>=0 && y<h && y>=0);
    return data[ch*w*h + y*w + x];
    }
  
  float& operator()(int x, int y)
    {
    assert(c==1 && x<w && x>=0 && y<h && y>=0);
    return data[y*w + x];
    }
  
  const float& operator()(int x, int y, int ch) const 
    {
    //printf("%d %d %d   %d %d %d\n",w,h,c,x,y,ch);
    assert(ch<c && ch>=0 && x<w && x>=0 && y<h && y>=0);
    return data[ch*w*h + y*w + x];
    }
  
  const float& operator()(int x, int y) const 
    {
    assert(c==1 && x<w && x>=0 && y<h && y>=0);
    return data[y*w + x];
    }
  
        float& pixel(int x, int y, int ch)       { return operator()(x,y,ch); }
  const float& pixel(int x, int y, int ch) const { return operator()(x,y,ch); }
        float& pixel(int x, int y)               { return operator()(x,y); }
  const float& pixel(int x, int y) const         { return operator()(x,y); }
  
  
  float get_pixel(int x, int y, int ch) const
    {
    assert(ch<c && ch>=0);
    
    if(x>=w)x=w-1;
    if(y>=h)y=h-1;
    if(x<0)x=0;
    if(y<0)y=0;
    return data[ch*w*h + y*w + x];
    }
  
  float get_pixel(int x, int y) const
    {
    assert(c==1);
    
    if(x>=w)x=w-1;
    if(y>=h)y=h-1;
    if(x<0)x=0;
    if(y<0)y=0;
    return data[y*w + x];
    }
  
  void set_pixel(int x, int y, int ch, float v)
    {
    if(x>=w)return;
    if(y>=h)return;
    if(ch>=c)return;
    if(x<0)return;
    if(y<0)return;
    if(ch<0)return;
    data[ch*w*h + y*w + x]=v;
    }
  
  const float* RowPtr(int row, int channel) const { return data+channel*w*h+row*w; }
        float* RowPtr(int row, int channel)       { return data+channel*w*h+row*w; }
  
  bool contains(float x, float y) const { return x>-0.5f && x<w-0.5f && y>-0.5f && y<h-0.5f; }
  bool is_empty(int x, int y) const
    {
    assert(x<w && x>=0 && y<h && y>=0);
    for(int q1=0;q1<c;q1++)if(pixel(x,y,q1))return false;
    return true;
    }
  
  bool is_nonempty_patch(int x, int y, int w=0) const
    {
    for(int q1=x-w;q1<=x+w;q1++)for(int q2=y-w;q2<=y+w;q2++)
      {
      int c1=0;
      for(int ch=0;ch<c;ch++)if(get_pixel(q1,q2,ch))c1++;
      if(c1==0)return false;
      }
    return true;
    }
  
  float nn_interpolate(float x, float y, int c) const;
  float bilinear_interpolate(float x, float y, int c) const;
  
  int size(void) const { return w*h*c; }
  void clear(void) const { memset(data,0,sizeof(float)*c*w*h); }
  Image abs(void) const;
  
  // member functions for modifying image
  void l1_normalize(void);
  void RGBtoHSV(void);
  void HSVtoRGB(void);
  void LCHtoRGB(void);
  void RGBtoLCH(void);
  void shift(int c, float v);
  void scale(int c, float v);
  void threshold(float thres);
  void clamp(void);
  void feature_normalize(void);
  void feature_normalize_total(void);
  
  Image get_channel(int c) const;
  void set_channel(int c,const Image& im);
  Image rgb_to_grayscale(void) const;
  Image transpose(void) const;
  
  void save(const string& file)
    {
    FILE*fn=fopen(file.c_str(),"wb");
    fwrite(&w,sizeof(w),1,fn);
    fwrite(&h,sizeof(h),1,fn);
    fwrite(&c,sizeof(c),1,fn);
    fwrite(data,sizeof(float),size(),fn);
    fclose(fn);
    }
  
  static Image load(const string& file)
    {
    
    int w,h,c;
    FILE*fn=fopen(file.c_str(),"rb");
    fread(&w,sizeof(w),1,fn);
    fread(&h,sizeof(h),1,fn);
    fread(&c,sizeof(c),1,fn);
    Image im(w,h,c);
    fread(im.data,sizeof(float),im.size(),fn);
    fclose(fn);
    return im;
    }
  };

inline float get_pixel(const Image& im, int x, int y, int c)          { return im.get_pixel(x,y,c); }
inline void  set_pixel(      Image& im, int x, int y, int c, float v) {        im.set_pixel(x,y,c,v); }
inline Image make_image(int w,int h, int c){return Image(w,h,c);}

bool operator ==(const Image& a, const Image& b);

// A 2d point.
// float x, y: the coordinates of the point.
struct Point 
  {
  double x, y;
  
  Point() : x(0), y(0) {}
  Point(double x, double y) : x(x), y(y) {}
  };

// A descriptor for a point in an image.
// point p: x,y coordinates of the image pixel.
// vector<float> data: the descriptor for the pixel.
struct Descriptor
  {
  Point p;
  vector<float> data;
  
  Descriptor(){}
  Descriptor(const Point& p) : p(p) {}
  };

// A match between two points in an Image.
// const Descriptor* a, b: Pointers to the Descriptors in the corresponding images.
// float distance: the distance between the descriptors for the points.
struct Match
  {
  const Descriptor* a=nullptr;
  const Descriptor* b=nullptr;
  float distance=0.f;
  
  Match(){}
  Match(const Descriptor* a,const Descriptor* b,float dist=0.f) : a(a), b(b), distance(dist) {}
  
  bool operator<(const Match& other) { return distance<other.distance; }
  };


struct Color
  {
  union 
    {
    struct { float red,green,blue,alpha; };
    struct { float r,g,b,a; };
    float c[4];
    };
  
  Color(float r,float g,float b,float a=1.0) : red(r), green(g), blue(b), alpha(a) {}
  Color() : red(0), green(0), blue(0), alpha(1) {}
  
  static Color HSV(float hue, float sat = 1.0f, float val = 1.0f, float alpha = 1.0f)
    {
    float h = 6.0f * hue;
    if(h==6.f)h=h-1;
    int i = (int)floor(h);
    float f = (i%2 == 0) ? 1-(h-i) : h-i;
    float m = val * (1-sat);
    float n = val * (1-sat*f);
    
    switch(i)
      {
      case 0: return Color(val,n,m,alpha);
      case 1: return Color(n,val,m,alpha);
      case 2: return Color(m,val,n,alpha);
      case 3: return Color(m,n,val,alpha);
      case 4: return Color(n,m,val,alpha);
      case 5: return Color(val,m,n,alpha);
      default: throw std::runtime_error("Found extra colour in rainbow.");
      }
    }
  
  
  };



struct LKIterPyramid
  {
  // INPUT
  vector<Image> pyramid1;
  vector<Image> pyramid0;
  Image t0;
  Image t1;
  
  // OUTPUT
  Image v;  // resulting velocity
  Image colorflow; // colorized velocity
  Image error; // difference between warp(t0,v) and t1
  Image ev3; // three channel eigenvalue image for displaying
  Image warped;// warped t0 with flow v
  Image all; // ciombined 4 images for Opencv display
  
  // OPTIONS
  float subsample_input=2;    // how much to reduce input image size
  float smooth_structure=1;   // how much to smooth structure matrix
  float smooth_vel=1;         // how much to smooth resulting velocity
  int lk_iterations=2;        // LK iterations to run (0 -  no flow, 1 - standard version)
  int pyramid_levels=6;       // pyramid levels (1 - standard algo)
  float pyramid_factor=2;     // ratio of sizes between successive pyramid levels
  float clamp_vel=10;         // how much to clamp velocity
  float vel_color_scale=4;    // saturation of vel image
  
  bool compute_all=false; // compute total combined image for opencv
  bool compute_colored_ev=true; // compute colored eigenvalue image
  };



Image load_image(const string& filename);
void save_png  (const Image& im, const string& name);
void save_image(const Image& im, const string& name);


// Basic operations
Image rgb_to_grayscale(const Image& im);
Image grayscale_to_rgb(const Image& im, float r, float g, float b);
Image vel2rgb(const Image& v, float thres);
void rgb_to_hsv(Image& im);
void hsv_to_rgb(Image& im);
void lch_to_rgb(Image& im);
void rgb_to_lch(Image& im);
void shift_image(Image& im, int c, float v);
void scale_image(Image& im, int c, float v);
void clamp_image(Image& im);
void l1_normalize(Image& im);

Image get_channel(const Image& im, int c);
Image sub_image(const Image& a, const Image& b);
Image add_image(const Image& a, const Image& b);
Image operator-(const Image& a, const Image& b);
Image operator+(const Image& a, const Image& b);

int same_image(const Image& a, const Image& b);

// Resizing
float nn_interpolate(const Image& im, float x, float y, int c);
float bilinear_interpolate(const Image& im, float x, float y, int c);

Image nn_resize(const Image& im, int w, int h);
Image bilinear_resize(const Image& im, int w, int h);


// Filtering
Image fast_smooth_image(const Image& im, float sigma); // USE THIS
Image convolve_image(const Image& im, const Image& filter, int preserve);
Image make_box_filter(int w);
Image make_highpass_filter(void);
Image make_sharpen_filter(void);
Image make_emboss_filter(void);
Image make_gaussian_filter(float sigma);
Image make_gx_filter(void);
Image make_gy_filter(void);
void feature_normalize(Image& im);
void feature_normalize_total(Image& im);
Image fast_smooth_image(const Image& im, float sigma);

void threshold_image(Image& im, float thresh);
pair<Image,Image> sobel_image(const Image&  im);
Image colorize_sobel(const Image&  im);
Image smooth_image(const Image&  im, float sigma);
Image bilateral_filter(const Image& im, float sigma, float sigma2);

// Harris and panorama
Image structure_matrix(const Image& im, float sigma);
Image cornerness_response(const Image& S, int method);
Image nms_image(const Image& im, int w);
vector<Descriptor> detect_corners(const Image& im, const Image& nms, float thresh, int window);
vector<Descriptor> harris_corner_detector(const Image& im, float sigma, float thresh, int window, int nms, int corner_method);
Image detect_and_draw_corners(const Image& im, float sigma, float thresh, int window, int nms, int corner_method);
Image mark_corners(const Image& im, const vector<Descriptor>& d);

// Panorama
Image both_images(const Image& a, const Image& b);
Image draw_matches(const Image& a, const Image& b, const vector<Match>& matches, const vector<Match>&  inliers);
Image draw_inliers(const Image& a, const Image& b, const Matrix& H, const vector<Match>& m, float thresh);
Image find_and_draw_matches(const Image& a, const Image& b, float sigma, float thresh, int window, int nms, int corner_method);
float l1_distance(const vector<float>& a,const vector<float>& b);
vector<Match> match_descriptors(const vector<Descriptor>& a,const vector<Descriptor>& b);
Point project_point(const Matrix& H, const Point& p);
double point_distance(const Point& p, const Point& q);
vector<Match> model_inliers(const Matrix& H, const vector<Match>& m, float thresh);
void randomize_matches(vector<Match>& m);
Matrix compute_homography_ba(const vector<Match>& matches);
Matrix RANSAC(vector<Match> m, float thresh, int k, int cutoff);
Image combine_images(const Image& a, const Image& b, const Matrix& Hba, float acoeff);
Image panorama_image(const Image& a, const Image& b, float sigma, int corner_method, float thresh, int window, int nms, float inlier_thresh, int iters, int cutoff, float acoeff);
Image cylindrical_project(const Image& im, float f);
Image spherical_project(const Image& im, float f);

// Optical Flow
Image optical_flow_images(const Image& im, const Image& prev, float smooth_win, float smooth_vel);
void optical_flow_webcam(void);
void draw_flow(Image& im, const Image& v, float scale);
Image warp_flow(const Image& im, const Image& v);
void constrain_image(const Image& im, float v);
Image velocity_resize(const Image& oldvel, int w, int h);

Image eigenvalue_matrix(const Image& ts);
Image time_structure_matrix(const Image& im, const Image& prev, float s);
Image velocity_image(const Image& S,const Image& ev);
vector<Image> make_image_pyramid(const Image& a, float factor, int levels);
void compute_iterative_pyramid_LK(LKIterPyramid& lk);

#ifdef OPENCV
#include <opencv2/opencv.hpp>
Image get_image_from_stream(cv::VideoCapture& cap);
cv::Mat Image2Mat(const Image& im);
Image Mat2Image(const cv::Mat& frame);
#endif

