cmake_minimum_required(VERSION 2.8)

project(cse455-hw0)


add_library(uwimg SHARED
     src/args.c
     src/args.h
     src/image.h
     src/load_image.c
     src/process_image.c
     src/stb_image.h
     src/stb_image_write.h
     )


link_libraries(uwimg m)


add_executable(test src/test.c src/test.h)
