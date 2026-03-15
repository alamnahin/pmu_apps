add_library(${PROJECT_NAME}-estimation STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src/estimator.cpp)

# Add the "include/" directory to the include directories
target_include_directories(${PROJECT_NAME}-estimation
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  ${PROJECT_NAME}-estimation
  PUBLIC fftw3
  PUBLIC fftw3f
  PUBLIC sdft-cpp)

add_executable(${PROJECT_NAME}-estimation-app
               ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(
  ${PROJECT_NAME}-estimation-app
  PRIVATE Boost::program_options
  PRIVATE ${PROJECT_NAME}-estimation)
