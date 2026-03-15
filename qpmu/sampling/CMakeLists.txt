set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(
  Boost 1.70.0
  COMPONENTS program_options
  REQUIRED)

add_executable(${PROJECT_NAME}-sampling-app)

target_sources(${PROJECT_NAME}-sampling-app
               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(${PROJECT_NAME}-sampling-app
                      PRIVATE Boost::program_options)
