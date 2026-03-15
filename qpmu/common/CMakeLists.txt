add_library(${COMMON_LIB} STATIC)

target_include_directories(${COMMON_LIB}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(${COMMON_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/util.cpp)
