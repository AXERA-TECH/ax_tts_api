# espeak-ng
set(ESPEAK_INC_DIR ${CMAKE_SOURCE_DIR}/third_party/espeak-ng/include)
set(ESPEAK_LIB_DIR ${CMAKE_SOURCE_DIR}/third_party/espeak-ng/lib)
list(APPEND ESPEAK_LIBS espeak-ng ucd speechPlayer pthread)

include_directories(${ESPEAK_INC_DIR})
link_directories(${ESPEAK_LIB_DIR})