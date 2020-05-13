#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "utils.h"
#include "image.h"

int tests_total = 0;
int tests_fail = 0;

int same_image(const Image& a, const Image& b) { return a==b; }

bool operator ==(const Image& a, const Image& b)
  {
  if(a.w != b.w || a.h != b.h || a.c != b.c) 
    {
    printf("Expected %d x %d x %d image, got %d x %d x %d\n", b.w, b.h, b.c, a.w, a.h, a.c);
    return 0;
    }
  
  for(int i = 0; i < a.w*a.h*a.c; ++i) if(!within_eps(a.data[i], b.data[i])) 
    {
    printf("The value at %d %d %d should be %f, but it is %f! \n", i/(a.w*a.h), (i%(a.w*a.h))/a.h, (i%(a.w*a.h))%a.h, b.data[i], a.data[i]);
    return 0;
    }
  return 1;
  }



template <size_t TSZ>
void TiledTranspose(Image& img_out, const Image& img_in, int c)
  {
  const size_t w = img_in.w;
  const size_t h = img_in.h;
  const size_t BPP = sizeof(float);
  
  float d[TSZ][TSZ];
  
  for(size_t xin = 0; xin < w; xin += TSZ)
    for(size_t yin = 0; yin < h; yin += TSZ)
      {
      const size_t xspan = min(TSZ, w - xin);
      const size_t yspan = min(TSZ, h - yin);
      const size_t dmin = min(xspan, yspan);
      const size_t dmax = max(xspan, yspan);
      const size_t xout = yin;
      const size_t yout = xin;
      
      for(size_t y = 0; y < yspan; y++)
        memcpy(d[y], &img_in(xin, yin + y, c), xspan * BPP);
      
      for(size_t x = 0; x < dmin; x++)
        for(size_t y = x + 1; y < dmax; y++)
          swap(d[x][y], d[y][x]);
      
      for(size_t y = 0; y < xspan; y++)
        memcpy(&img_out(xout, yout + y,  c), d[y], yspan * BPP);
      }
  }




Image Image::transpose(void) const
  {
  //TIME(1);
  Image ret(h,w,c);
  
  if(c>1)
    {
    vector<thread> th;
    for(int c=0;c<this->c;c++)th.push_back(thread([&ret,this,c](){TiledTranspose<80>(ret,*this,c);}));
    for(auto&e1:th)e1.join();
    }
  else TiledTranspose<80>(ret,*this,0);
  
  return ret;
  }


inline float dot_product(const float* a, const float* b, int n)
  {
  float sum=0;
  for(int q1=0;q1<n;q1++)sum+=a[q1]*b[q1];
  return sum;
  }


Image Image::abs(void) const 
  {
  Image ret=*this;
  for(int q2=0;q2<h;q2++)for(int q1=0;q1<w;q1++)
    for(int q3=0;q3<c;q3++)
      {
      float a=pixel(q1,q2,q3);
      ret(q1,q2,q3)=fabsf(a);
      }
  return ret;
  }



Image vel2rgb(const Image& v, float thres)
  {
  TIME(1);
  assert(v.c==2 && "velocity must contain 2 channels");
  Image ret(v.w,v.h,3);
  
  for(int q2=0;q2<v.h;q2++)for(int q1=0;q1<v.w;q1++)
    {
    float dx=v(q1,q2,0);
    float dy=v(q1,q2,1);
    //printf("%f %f %f\n",dx,dy,sqrtf(dx*dx+dy*dy));
    float mag=min(sqrtf(dx*dx+dy*dy)/thres,1.f);
    float hue=(atan2f(dy,dx)+M_PI)/2/M_PI;
    if(hue<0)hue=0;
    if(hue>1)hue=1;
    Color c=Color::HSV(hue,mag,mag);
    ret(q1,q2,0)=c.c[0];
    ret(q1,q2,1)=c.c[1];
    ret(q1,q2,2)=c.c[2];
    }
  return ret;
  }

Image Image::rgb_to_grayscale(void) const { return ::rgb_to_grayscale(*this); }

Image fast_smooth_image(const Image& im, float sigma)
  {
  //TIME(1);
  assert(sigma>=0.f);
  int w=roundf(sigma*6);
  if(w%2==0)w++;
  
  vector<float> g(w);
  float*gf=g.data()+w/2;
  
  {
  float sum=0;
  for(int q1=-w/2;q1<=w/2;q1++)gf[q1]=expf(-(q1*q1)/(2.f*sigma*sigma));
  for(int q1=-w/2;q1<=w/2;q1++)sum+=gf[q1];
  for(int q1=-w/2;q1<=w/2;q1++)gf[q1]/=sum;
  }
  
  
  auto do_one=[gf,w](const Image& im)
    {
    //TIME(1);
    Image expand(im.w+w-1,im.h,im.c);
    for(int c=0;c<im.c;c++)
    for(int q2=0;q2<expand.h;q2++)for(int q1=0;q1<expand.w;q1++)
      expand(q1,q2,c)=im.get_pixel(q1-w/2,q2,c);
    
    Image ret(im.w,im.h,im.c);
    
    int Nthreads=8;
    int total=im.h*im.c;
    vector<thread> th;
    for(int t=0;t<Nthreads;t++)th.push_back(thread([&](int a,int b)
      {
      for(int q=a;q<b;q++)
        {
        int c=q/im.h;
        int q2=q%im.h;
        for(int q1=0;q1<im.w;q1++)ret(q1,q2,c)=dot_product(&expand(q1,q2,c),gf-w/2,w);
        }
      },t*total/Nthreads,(t+1)*total/Nthreads));
    for(auto&e1:th)e1.join();
    
    return ret;
    };
  
  Image first=do_one(im);
  Image second=do_one(first.transpose()).transpose();
  
  
  return second;
  }




void Image::set_channel(int ch, const Image& im)
  {
  assert(im.c==1 && ch<c && ch>=0);
  assert(im.w==w && im.h==h);
  memcpy(&pixel(0,0,ch),im.data,sizeof(float)*im.size());
  }

Image Image::get_channel(int ch) const 
  {
  assert(ch<c && ch>=0);
  Image im(w,h,1);
  memcpy(im.data,&pixel(0,0,ch),sizeof(float)*im.size());
  return im;
  }
