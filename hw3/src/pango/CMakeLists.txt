
#link_libraries(uwimg++)

#link_directories(/home/swetko/local/Pangolin/build/src)
#include_directories(/home/swetko/local/Pangolin/build/src/include /home/swetko/local/Pangolin/include)

find_package(Pangolin QUIET)

if(Pangolin_FOUND)
  message(---------------------------------------------)
  message("Pangolin found!!!!")
  message("Building WITH Visualization")
  message(---------------------------------------------)
  
  link_libraries(${Pangolin_LIBRARIES})
  include_directories(${Pangolin_INCLUDE_DIRS})
  
  
  add_library(pango-utils SHARED pango.cpp pango.h)
  
  link_libraries(pango-utils)
  
  add_executable(harris       harris.cpp  )
  add_executable(panorama     panorama.cpp )
  add_executable(projections  proj.cpp  )
  add_executable(optical-flow optical-flow.cpp )
else()
  message(---------------------------------------------)
  message("Pangolin NOT found")
  message("Building WITHOUT Visualization")
  message(---------------------------------------------)
endif()
