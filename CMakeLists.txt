# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

###add_custom_command(
###    COMMAND python data2h.py data/index.html main/indexhtml.h
###    DEPENDS data/index.html
###    OUTPUT main/indexhtml.h
###)


#-------------- RUN THIS WHEN WORKING THE HTML FILE --------------------
#execute_process(COMMAND python data2h.py data/index.html main/indexhtml.h WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})



   
project(esp32gong)

#target_add_binary_data(${PROJECT_NAME}.elf main/indexhtml.h BINARY DEPENDS main/indexhtml.h)
