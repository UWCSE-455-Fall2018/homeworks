#include "../image.h"
#include "../utils.h"
#include "../matrix.h"

#include <string>

using namespace std;



void test_functions()
  {
  Image a = load_image("data/dog_a.jpg").rgb_to_grayscale();
  Image b = load_image("data/dog_b.jpg").rgb_to_grayscale();
  
  Image S = time_structure_matrix(b,a,10);
  Image ev = eigenvalue_matrix(S);
  Image vel = velocity_image(S,ev);
  Image colorflow = vel2rgb(vel,20);
  Image warped = warp_flow(a,vel);
  
  S.save("output/s.raw");
  ev.save("output/ev.raw");
  vel.save("output/vel.raw");
  colorflow.save("output/colorflow.raw");
  warped.save("output/warped.raw");
  
  Image S1(a.w,a.h,3);
  S1.set_channel(0,S.get_channel(0));
  S1.set_channel(1,S.get_channel(1));
  S1.set_channel(2,S.get_channel(2));
  Image S2(a.w,a.h,3);
  S2.set_channel(0,S.get_channel(3));
  S2.set_channel(1,S.get_channel(4));
  Image ev3(a.w,a.h,3);
  ev3.set_channel(0,ev.get_channel(0));
  ev3.set_channel(1,ev.get_channel(1));
  
  ev3.feature_normalize_total();
  S1.feature_normalize_total();
  S2.feature_normalize_total();
  
  
  
  save_png(S1,"output/StS");
  save_png(S2,"output/StT");
  save_png(ev3,"output/ev");
  save_png(colorflow,"output/colorflow");
  save_png(warped,"output/warped");
  
  Image Sraw   =Image::load("data/s.raw");
  Image EVraw  =Image::load("data/ev.raw");
  Image VELraw =Image::load("data/vel.raw");
  Image CFraw  =Image::load("data/colorflow.raw");
  Image WARPraw=Image::load("data/warped.raw");
  
  S.save("output/s.raw");
  ev.save("output/ev.raw");
  vel.save("output/vel.raw");
  colorflow.save("output/colorflow.raw");
  warped.save("output/warped.raw");

  
  TEST(same_image(S,Sraw));
  TEST(same_image(ev,EVraw));
  TEST(same_image(vel,VELraw));
  TEST(same_image(colorflow,CFraw));
  TEST(same_image(warped,WARPraw));
  printf("%d tests, %d passed, %d failed\n", tests_total, tests_total-tests_fail, tests_fail);
  
  
  
  }






// Run optical flow demo on webcam
void optical_flow_webcam(void)
  {
  LKIterPyramid lk;
  lk.compute_all=true;
  lk.compute_colored_ev=false;
  
  
#ifdef OPENCV
  cv::VideoCapture cap(0);
  assert(cap.isOpened() && "cannot open camera");
  Image imo[2]; // original full size image
  Image im[2]; // resized image
  vector < Image > pyramid[2]; // pyramids
  
  int cur=1; // current latest image (1-cur is the previous image)
  
  imo[cur]=get_image_from_stream(cap);
  
  int w=imo[cur].w/lk.subsample_input;
  int h=imo[cur].h/lk.subsample_input;
  
  im[cur]=bilinear_resize(fast_smooth_image(imo[cur],lk.subsample_input/2),w,h);
  pyramid[cur]=make_image_pyramid(im[cur].rgb_to_grayscale(),lk.pyramid_factor,lk.pyramid_levels);
  
  
  
  
  //Image all(w*2,h*2,3);
  //Image warped,v;
  
  while(1)
    {
    cur=1-cur;
    
    imo[cur]=get_image_from_stream(cap);
    
    im[cur]=bilinear_resize(fast_smooth_image(imo[cur],lk.subsample_input/2),w,h);
    pyramid[cur]=make_image_pyramid(im[cur].rgb_to_grayscale(),lk.pyramid_factor,lk.pyramid_levels);
    
    {
    lk.t1=im[cur];               // init
    lk.t0=im[1-cur];             // init
    lk.pyramid1=pyramid[cur];    // init
    lk.pyramid0=pyramid[1-cur];  // init
    }
    
    compute_iterative_pyramid_LK(lk);
    
    cv::imshow("Optical-Flow",Image2Mat(lk.all));
    if( cv::waitKey(1) == 27 ) break; // stop capturing by pressing ESC 
    
    
    
    //Image copy=im;
    //Image v = optical_flow_images(im_c.rgb_to_grayscale(), prev_c.rgb_to_grayscale(), 2,2);
    ////draw_flow(copy, v, smooth*div);
    ////save_image(copy,"copy");
    //int key = show_image(copy, "flow", 5);
    //prev=move(im);
    //prev_c=move(im_c);
    //
    //if(key != -1){key = key % 256;printf("%d\n", key);if (key == 27) break;}
    //
    //im = get_image_from_stream(cap);
    //im_c = nn_resize(im, im.w/div, im.h/div);
    }
#else
    fprintf(stderr, "Must compile with OpenCV\n");
#endif
  }

int main(int argc, char **argv)
  {
  if(argc==2 && string(argv[1])=="live")
    {
    optical_flow_webcam();
    return 0;
    }
  
  test_functions();
  
  
  
  Image a = load_image("data/dog_a.jpg");
  Image b = load_image("data/dog_b.jpg");
  Image ag = a.rgb_to_grayscale();
  Image bg = b.rgb_to_grayscale();
  Image flow = optical_flow_images(bg, ag,5,5);
  Image colorflow=vel2rgb(flow,10);
  save_image(colorflow, "output/dog_vel");
  
  LKIterPyramid lk;
  lk.pyramid_levels=8;
  lk.vel_color_scale=20;
  lk.clamp_vel=50;
  
  lk.t1=b;
  lk.t0=a;
  lk.pyramid0=make_image_pyramid(ag,lk.pyramid_factor,lk.pyramid_levels);
  lk.pyramid1=make_image_pyramid(bg,lk.pyramid_factor,lk.pyramid_levels);
  
  compute_iterative_pyramid_LK(lk);
  
  save_image(lk.colorflow, "output/dog_vel_improved");
  
  
  return 0;
  }
