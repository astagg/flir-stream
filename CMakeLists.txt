cmake_minimum_required(VERSION 3.12)
project(stream)

# Set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)

# Find the OpenCV library
find_package(OpenCV REQUIRED)

# Find the GStreamer library
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4)

include_directories(
    ${Spinnaker_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

# Set the include directories for the Spinnaker library
set(Spinnaker_INCLUDE_DIRS "/opt/spinnaker/include")

# Set the libraries for the Spinnaker library
set(Spinnaker_LIBRARIES "/opt/spinnaker/lib/libSpinnaker.so")


# Add the executable
add_executable(stream stream.cpp)

# Link the executable with the OpenCV, GStreamer, and Spinnaker libraries
target_link_libraries(stream ${OpenCV_LIBS} ${GST_LIBRARIES} ${Spinnaker_LIBRARIES})

# Set the include directories for the OpenCV, GStreamer, and Spinnaker libraries
target_include_directories(stream PUBLIC ${OpenCV_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${Spinnaker_INCLUDE_DIRS})

