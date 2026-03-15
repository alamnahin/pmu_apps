# Suppress warnings from external libraries
if(MSVC)
  add_compile_options(/W0)
else()
  add_compile_options(-w)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/github.com/jurihock/sdft/cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/github.com/tahmid-khan/STRONgrid)
