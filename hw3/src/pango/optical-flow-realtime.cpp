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

#include <thread>
#include "../image.h"
#include "pango.h"

using namespace  std;

std::mutex control_mutex;
pangolin::VideoInput video;
pangolin::VideoPlaybackInterface* video_playback;
pangolin::VideoInterface* video_interface;
int grab_until=std::numeric_limits<int>::max();
int current_frame=-1;
bool video_grab_wait=true;
bool video_grab_newest=false;

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
  
  if(TotalFrames() < std::numeric_limits<int>::max() )
    {
    std::cout << "Video length: " << TotalFrames() << " frames" << std::endl;
    grab_until = 0;
    }
  
  pangolin::Var<int>::Attach("ui.frame", current_frame);
  pangolin::Var<int> frame("ui.frame");
  frame.Meta().range[0] = 1;
  frame.Meta().range[1] = TotalFrames()-1;
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


void Skip(int frames)
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  if(video_playback)
    {
    const int next_frame = current_frame + frames;
    if (next_frame >= 0)
      {
      current_frame = video_playback->Seek(next_frame) -1;
      grab_until = current_frame + 1;
      } 
    }
  else
    {
    if(frames >= 0)grab_until = current_frame + frames;
    else pango_print_warn("Unable to skip backward.");
    }
  }

void TogglePlay()
  {
  std::lock_guard<std::mutex> lock(control_mutex);
  grab_until = (current_frame < grab_until) ? current_frame: std::numeric_limits<int>::max();
  }


void RegisterDefaultKeyShortcutsAndPangoVariables()
  {
  pangolin::RegisterKeyPressCallback(' ', [](){TogglePlay();} );
  pangolin::RegisterKeyPressCallback('w', [](){ToggleWaitForFrames();} );
  pangolin::RegisterKeyPressCallback('d', [](){ToggleDiscardBufferedFrames();} );
  pangolin::RegisterKeyPressCallback(',', [](){Skip(-1);} );
  pangolin::RegisterKeyPressCallback('.', [](){Skip(+1);} );
  pangolin::RegisterKeyPressCallback('<', [](){Skip(-20);} );
  pangolin::RegisterKeyPressCallback('>', [](){Skip(+20);} );
  pangolin::RegisterKeyPressCallback('0', [](){pangolin::SaveFramebuffer("output/screenshot",pangolin::DisplayBase().v);} );
  }



int main(int argc, char** argv)
  {
  if(argc!=2)
    {
    printf("USAGE: ./optical-flow URI\nURI=uvc://\nURI=test.pango\n...etc.\n");
    return 0;
    }
  
  OpenInput(argv[1]);
  
  assert(video.Streams().size()==1 && "Can only work with single stream videos\n");
  
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
  
  pangolin::View& container = pangolin::Display("container");
  container.SetLayout(pangolin::LayoutEqual);
  container.SetBounds(pangolin::Attach::Pix(slider_size),1,
                      pangolin::Attach::Pix(UI_WIDTH),1);
  
    
  vector<pangolin::ImageView> iv(1);
  for(pangolin::ImageView& e1:iv)container.AddDisplay(e1);
  
  
  // Safe and efficient binding of named variables.
  pangolin::Var<float> focal_length_pix("ui.focal_length_pix",500,10,10000,true);
  pangolin::Var<std::function<void(void)>>("ui.SaveScreenshot", [&](void){ pangolin::SaveFramebuffer("output/screenshot",pangolin::DisplayBase().v); });
  
  
  
  
  std::vector<pangolin::Image<unsigned char> > camimages;
  std::unique_ptr<unsigned char[]> cambuffer(new unsigned char[video.SizeBytes()+1]);
  
  video.Start();
  RegisterDefaultKeyShortcutsAndPangoVariables();
  
  pangolin::FlagVarChanged();
  
  
  // Default hooks for exiting (Esc) and fullscreen (tab).
  while(!pangolin::ShouldQuit())
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1.0,1.0,1.0);
    
    if(frame<1){frame=1;pangolin::FlagVarChanged();}
    
    //if(pangolin::GuiVarHasChanged())run_pipeline();
    
    if(frame.GuiChanged()) 
      {
      if(video_playback) frame = video_playback->Seek(frame) -1;
      grab_until = frame + 1;
      }
    
    if ( frame < grab_until && video.Grab(&cambuffer[0], camimages, video_grab_wait, video_grab_newest))
      {
      frame = frame + 1;
      iv[0].SetImage(camimages[0], pangolin::GlPixFormat(video.Streams()[0].PixFormat() ));
      }
    
    
    //iv[0].SetImage(toPangolin(image));
    
    auto draw_overlay=[&](pangolin::View& view)
      {
      };
    
    iv[0].SetDrawFunction(draw_overlay);
    
    // leave in pixel orthographic for slider to render.
    pangolin::DisplayBase().ActivatePixelOrthographic();
    pangolin::FinishFrame();
    }
  
  return 0;
  }
