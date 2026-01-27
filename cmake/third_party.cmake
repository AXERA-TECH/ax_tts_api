# third party
set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# espeak-ng
set(ESPEAK_INC_DIR ${THIRDPARTY_DIR}/espeak-ng/include)
set(ESPEAK_LIB_DIR ${THIRDPARTY_DIR}/espeak-ng/lib)
list(APPEND ESPEAK_LIBS espeak-ng ucd speechPlayer pthread)

include_directories(${ESPEAK_INC_DIR})
link_directories(${ESPEAK_LIB_DIR})

# onnxruntime
set(ORT_INC_DIR ${THIRDPARTY_DIR}/onnxruntime-linux-aarch64-static_lib-1.16.0/include)
set(ORT_LIB_DIR ${THIRDPARTY_DIR}/onnxruntime-linux-aarch64-static_lib-1.16.0/lib)
list(APPEND ORT_LIBS onnxruntime)

include_directories(${ORT_INC_DIR})
link_directories(${ORT_LIB_DIR})