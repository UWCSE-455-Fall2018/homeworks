#include <pangolin/pangolin.h>
#include <pangolin/display/image_view.h>
#include <pangolin/video/video_input.h>
#include <pangolin/display/window.h>
#include <pangolin/platform.h>
#include <pangolin/display/image_view.h>
#include <pangolin/gl/glpixformat.h>
#include <pangolin/gl/gltexturecache.h>
#include <pangolin/handler/handler_image.h>
#include <pangolin/pangolin.h>
#include <pangolin/utils/file_utils.h>
#include <pangolin/utils/sigstate.h>
#include <pangolin/utils/timer.h>
#include <pangolin/video/video_input.h>

#include <memory>

#include <thread>
#include "../image.h"
#include "pango.h"

using namespace  std;

namespace pangolin
{
void SaveFramebuffer(VideoOutput& video, const Viewport& v);
}


std::mutex control_mutex;
pangolin::VideoInput video;
pangolin::VideoPlaybackInterface* video_playback;
pangolin::VideoInterface* video_interface;
unique_ptr<pangolin::VideoOutput> vo;

int play=0;
int direction=1;
int current_frame=1;
bool change_direction=false;

bool video_grab_wait=true;
bool video_grab_newest=true;


int TotalFrames(void)
  {
  return video_playback ? video_playback->GetTotalFrames() : std::numeric_limits<int>::max();
  }
    
void OpenInput(const std::string& input_uri)
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  video.Open(input_uri, "video.pango");
  
  // Output details of video stream
  for(size_t s = 0; s < video.Streams().size(); ++s)
    {
    const pangolin::StreamInfo& si = video.Streams()[s];
    std::cout << pangolin::FormatString("Stream %: % x % % (pitch: % bytes)",s, si.Width(), si.Height(), si.PixFormat().format, si.Pitch()) << std::endl;
    }
  
  if(video.Streams().size() == 0)
    {
    pango_print_error("No video streams from device.\n");
    return;
    }
  
  video_playback = pangolin::FindFirstMatchingVideoInterface<pangolin::VideoPlaybackInterface>(video);
  video_interface = pangolin::FindFirstMatchingVideoInterface<pangolin::VideoInterface>(video);
  
  pangolin::Var<int>::Attach("ui.frame", current_frame);
  pangolin::Var<int> frame("ui.frame");
  frame.Meta().range[0] = 1;
  frame.Meta().range[1] = TotalFrames()-1;
  }


void Skip(int frames)
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  current_frame+=frames;
  }

void TogglePlay()
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  play=!play;
  if(play)pango_print_info("Video Palying.\n");
  else    pango_print_info("Video Stopped.\n");
  }
void ToggleDiscardBufferedFrames()
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  video_grab_newest = !video_grab_newest;
  if(video_grab_newest) pango_print_info("Discarding old frames.\n");
  else                  pango_print_info("Not discarding old frames.\n");
  }

void ToggleWaitForFrames()
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  video_grab_wait = !video_grab_wait;
  if(video_grab_wait)pango_print_info("Gui wait's for video frame.\n");
  else               pango_print_info("Gui doesn't wait for video frame.\n");
  }



void RegisterDefaultKeyShortcutsAndPangoVariables()
  {
  pangolin::RegisterKeyPressCallback(' ', [](){TogglePlay();} );
  pangolin::RegisterKeyPressCallback('w', [](){ToggleWaitForFrames();} );
  pangolin::RegisterKeyPressCallback('d', [](){ToggleDiscardBufferedFrames();} );
  pangolin::RegisterKeyPressCallback(',', [](){Skip(-1);if(change_direction)direction=-1;} );
  pangolin::RegisterKeyPressCallback('.', [](){Skip(+1);if(change_direction)direction=+1;} );
  pangolin::RegisterKeyPressCallback('<', [](){Skip(-20);if(change_direction)direction=-1;} );
  pangolin::RegisterKeyPressCallback('>', [](){Skip(+20);if(change_direction)direction=+1;} );
  pangolin::RegisterKeyPressCallback('0', [](){pangolin::SaveFramebuffer("output/screenshot",pangolin::DisplayBase().v);} );
  }



int main(int argc, char** argv)
  {
  printf("USAGE: ./optical-flow test.pango\n/optical-flow (no args for live)\n");
  
  int live=1;
  
  
    
  if(argc==2)
    {
    OpenInput(argv[1]);
    live=0;
    }
  else if(argc==3)
    {
    live=-1;
    }
  
  if(live==0)assert(video.Streams().size()==1 && "Can only work with single stream videos\n");
  
  // Create OpenGL window in single line
  pangolin::CreateWindowAndBind("Main",1280,720);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  const int UI_WIDTH = 200;
  const int slider_size = (TotalFrames() < std::numeric_limits<int>::max() ? 20 : 0);
  
  // Add named Panel and bind to variables beginning 'ui'
  // A Panel is just a View with a default layout and input handling
  pangolin::CreatePanel("ui").SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(UI_WIDTH));
  
  pangolin::Var<int> frame  ("ui.frame");
  pangolin::Slider frame_slider("frame", frame.Ref() );
  
  
  
  if(video_playback && TotalFrames() < std::numeric_limits<int>::max())
    {
    // frame_slider should be added first so that it can be rendered correctly.
    pangolin::DisplayBase().AddDisplay(frame_slider);
    frame_slider.SetBounds(0.0, pangolin::Attach::Pix(slider_size), 0.0, 1.0);
    }
  
  play=live;
  
  pangolin::View& container = pangolin::Display("container");
  container.SetLayout(pangolin::LayoutEqual);
  container.SetBounds(pangolin::Attach::Pix(slider_size),1,
                      pangolin::Attach::Pix(UI_WIDTH),1);
  
    
  vector<pangolin::ImageView> iv(6);
  for(pangolin::ImageView& e1:iv)container.AddDisplay(e1);
  
  
  // Safe and efficient binding of named variables.
  pangolin::Var<bool>::Attach("ui.change_direction", change_direction, true);
  pangolin::Var<float> vel_color_scale("ui.vel_color_scale",5,0.01,20,true);
  pangolin::Var<float> smooth_structure("ui.smooth_structure",1,1,10,true);
  pangolin::Var<float> smooth_vel("ui.smooth_vel",1,0,5);
  pangolin::Var<float> clamp_vel("ui.clamp_vel",10,0,10);
  pangolin::Var<bool> show_vel_image("ui.show_vel_image",true,0,1);
  pangolin::Var<bool> show_eigenvalue_image("ui.show_eigenvalue_image",true,0,1);
  pangolin::Var<float> subsample_input("ui.subsample_input",1,1,10,true);
  pangolin::Var<int> visual_subsample("ui.visual_subsample",4,1,16,true);
  pangolin::Var<bool> show_vector("ui.show_vector",true,0,1);
  pangolin::Var<bool> show_vector_end("ui.show_vector_end",false,0,1);
  pangolin::Var<int> lk_iterations("ui.lk_iterations",2,0,16);
  pangolin::Var<int> pyramid_levels("ui.pyramid_levels",6,1,10);
  pangolin::Var<float> pyramid_factor("ui.pyramid_factor",2,1,4);
  pangolin::Var<std::function<void(void)>>("ui.SaveScreenshot", [&](void){ pangolin::SaveFramebuffer("output/screenshot",pangolin::DisplayBase().v); });
  pangolin::Var<std::function<void(void)>>("ui.ToggleRecord", [&](void)
    { 
    if(vo)vo.reset();
    else
      {
      vo.reset(new pangolin::VideoOutput("pango://recording.pango"));
      vo->AddStream(pangolin::PixelFormatFromString("RGB24"),pangolin::DisplayBase().v.w,pangolin::DisplayBase().v.h);
      vo->SetStreams();
      play=1;
      }
    });
  
  
  auto get_frame=[&](int fn)
    {
    
    if(live==0)
      {
      std::vector<pangolin::Image<unsigned char> > images;
      std::unique_ptr<unsigned char[]> buffer(new unsigned char[video.SizeBytes()+1]);
      video_playback->Seek(fn);
      video.Grab(&buffer[0], images, true, false);
      return fromPangolin(images[0],video.Streams()[0].PixFormat());
      }
    else if(live==1)
      {
      #ifdef OPENCV
      static cv::VideoCapture cap(0); // open the default camera
      assert(cap.isOpened()   &&  "Could not open camera\n"); // check if we succeeded
      return get_image_from_stream(cap);
      #else
      return Image(1,1,1);
      #endif
      }
    else
      {
      return Image(1,1,1);
      }
    };
  
  
  
  Image t0,t1,v,colorflow,g0,g1,warpg0,error,t1orig,t0orig;
  
  if(live==-1)t0=load_image(argv[1]);
  if(live==-1)t1=load_image(argv[2]);
    
  if(live==0)video.Start();
  RegisterDefaultKeyShortcutsAndPangoVariables();
  
  
  
  
  // Default hooks for exiting (Esc) and fullscreen (tab).
  while(!pangolin::ShouldQuit())
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1.0,1.0,1.0);
    
    
    iv[2].Show(show_vel_image);
    iv[5].Show(show_eigenvalue_image);
    
    static int prevfn=-1;
    bool redo=false;
    redo|=prevfn!=frame;
    redo|=vel_color_scale.GuiChanged();
    redo|=subsample_input.GuiChanged();
    redo|=smooth_structure.GuiChanged();
    redo|=visual_subsample.GuiChanged();
    redo|=lk_iterations.GuiChanged();
    redo|=smooth_vel.GuiChanged();
    redo|=clamp_vel.GuiChanged();
    redo|=pyramid_factor.GuiChanged();
    redo|=pyramid_levels.GuiChanged();
    
    if(live==1 && play)
      {
      swap(t0,t1);
      t1=get_frame(0);
      if(t0.size()==0)redo=false;
      else redo=true;
      }
    
    if(live==1)if(t0.size()==0 || t1.size()==0)redo=false;
    
    //printf("%zu %zu %d %d\n",camimages0.size(),camimages1.size(),live,play);
    if(redo)
      {
      TIME(1);
      
      if(live==0)
        {
        t0=get_frame(frame-direction);
        t1=get_frame(frame);
        }
      
      int w=t0.w/subsample_input;
      int h=t0.h/subsample_input;
      
      vector < Image > t0p;
      vector < Image > t1p;
      
      auto do_one=[&](const Image& t, Image& t1)
        {
        t1=bilinear_resize(fast_smooth_image(t,subsample_input/2),w,h);
        return make_image_pyramid(t1.rgb_to_grayscale(),pyramid_factor,pyramid_levels);
        };
      
      {
      TIME(1);
      thread th1([&](){t0p=do_one(t0,t0orig);});
      thread th2([&](){t1p=do_one(t1,t1orig);});
      th1.join();th2.join();
      }
      
      Image S,ev,v2;
      
      for(int q2=pyramid_levels-1;q2>=0;q2--)
        {
        
        g0=t0p[q2];
        g1=t1p[q2];
        
        
        if(q2==pyramid_levels-1)
          {
          v=Image(g0.w,g0.h,2);
          warpg0=g0;
          }
        else
          {
          v=velocity_resize(v,g0.w,g0.h);
          warpg0=warp_flow(g0,v);
          }
        
        for(int q1=0;q1<lk_iterations;q1++)
          {
          //v=v+optical_flow_images(g1, warpg0, smooth_structure, smooth_vel);
          S = time_structure_matrix(g1, warpg0, smooth_structure);
          ev = eigenvalue_matrix(S);
          v2 = velocity_image(S, ev);
          
          if(smooth_vel)v2=fast_smooth_image(v2,smooth_vel);
          v=v+v2;
          
          constrain_image(v,clamp_vel);
          if(q1<lk_iterations-1)warpg0=warp_flow(g0,v);
          }
        
        }
      
      warpg0=warp_flow(g0,v);
      colorflow=vel2rgb(v,vel_color_scale);
      error=(warpg0-g1).abs();
      
      Image ev3(ev.w,ev.h,3);
      memcpy(ev3.data,ev.data,ev.size()*sizeof(float));
      
      
      iv[0].SetImage(toPangolin(t0orig));
      iv[1].SetImage(toPangolin(t1orig));
      
      iv[2].SetImage(toPangolin(colorflow));
      iv[3].SetImage(toPangolin(warpg0));
      iv[4].SetImage(toPangolin(error));
      iv[5].SetImage(toPangolin(ev3));
      
      prevfn=frame;
      
      
      }
    
    
    
    auto draw_overlay=[&](pangolin::View& view)
      {
      for(int q2=0;q2<v.h;q2+=visual_subsample)for(int q1=0;q1<v.w;q1+=visual_subsample)
        {
        int x=q1;
        int y=q2;
        if(x>=t0.w)x=t0.w-1;
        if(y>=t0.h)x=t0.h-1;
        
        float x1=x+v(q1,q2,0);
        float y1=y+v(q1,q2,1);
        
        glColor3f(colorflow(q1,q2,0),colorflow(q1,q2,1),colorflow(q1,q2,2));
        if(show_vector)pangolin::glDrawLine(x,y,x1,y1);
        if(show_vector_end)pangolin::glDrawCross(x1,y1,0.25);
        }
      };
    
    iv[0].SetDrawFunction(draw_overlay);
    
    
    
    // leave in pixel orthographic for slider to render.
    pangolin::DisplayBase().ActivatePixelOrthographic();
    pangolin::FinishFrame();
    
    
    if(vo)pangolin::SaveFramebuffer(*vo,pangolin::DisplayBase().v); 
    
    
    if(live==0 && play)frame=frame+direction;
    if(live==0 && frame>frame.Meta().range[1] || frame<frame.Meta().range[0])
      {
      if(frame>frame.Meta().range[1])frame=frame.Meta().range[1];
      if(frame<frame.Meta().range[0])frame=frame.Meta().range[0];
      if(vo)vo.reset();
      play=false;
      }
    }
  
  return 0;
  }
