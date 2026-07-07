# third party
set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# Distinguish arm-uclibc vs arm-gnueabihf by compiler target.
set(AX_TTS_COMPILER_TARGET "")
if(CMAKE_C_COMPILER)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dumpmachine
        OUTPUT_VARIABLE AX_TTS_COMPILER_TARGET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT AX_TTS_COMPILER_TARGET)
    set(AX_TTS_COMPILER_TARGET "${CMAKE_C_COMPILER}")
endif()

set(AX_TTS_THIRDPARTY_FLAVOR "")
if(AX_TTS_COMPILER_TARGET MATCHES "aarch64")
    set(AX_TTS_THIRDPARTY_FLAVOR "aarch64")
elseif(AX_TTS_COMPILER_TARGET MATCHES "uclibc")
    set(AX_TTS_THIRDPARTY_FLAVOR "arm_uclibc")
elseif(AX_TTS_COMPILER_TARGET MATCHES "gnueabihf")
    set(AX_TTS_THIRDPARTY_FLAVOR "arm_gnueabihf")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(AX_TTS_THIRDPARTY_FLAVOR "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    set(AX_TTS_THIRDPARTY_FLAVOR "arm_uclibc")
else()
    message(FATAL_ERROR "Unsupported processor/toolchain: ${CMAKE_SYSTEM_PROCESSOR}, target=${AX_TTS_COMPILER_TARGET}")
endif()

message(STATUS "AX_TTS third-party flavor: ${AX_TTS_THIRDPARTY_FLAVOR} (target=${AX_TTS_COMPILER_TARGET})")

# espeak-ng
set(ESPEAK_INC_DIR ${THIRDPARTY_DIR}/espeak-ng/include)
if(AX_TTS_THIRDPARTY_FLAVOR STREQUAL "aarch64")
    set(ESPEAK_LIB_DIR ${THIRDPARTY_DIR}/espeak-ng/lib/aarch64)
elseif(AX_TTS_THIRDPARTY_FLAVOR STREQUAL "arm_uclibc")
    set(ESPEAK_LIB_DIR ${THIRDPARTY_DIR}/espeak-ng/lib/uclibc)
elseif(AX_TTS_THIRDPARTY_FLAVOR STREQUAL "arm_gnueabihf")
    set(ESPEAK_LIB_DIR ${THIRDPARTY_DIR}/espeak-ng/lib/gnueabihf)
else()
    message(FATAL_ERROR "Unknown third-party flavor: ${AX_TTS_THIRDPARTY_FLAVOR}")
endif()
if(NOT EXISTS ${ESPEAK_LIB_DIR})
    message(FATAL_ERROR "espeak lib dir not found: ${ESPEAK_LIB_DIR}. Run download_bsp.sh first.")
endif()
list(APPEND ESPEAK_LIBS espeak-ng ucd speechPlayer pthread)

include_directories(${ESPEAK_INC_DIR})
link_directories(${ESPEAK_LIB_DIR})

# onnxruntime
set(ORT_INC_DIR ${THIRDPARTY_DIR}/onnxruntime-linux-aarch64-static_lib-1.16.0/include)
if(AX_TTS_THIRDPARTY_FLAVOR STREQUAL "aarch64")
    set(ORT_LIB_DIR ${THIRDPARTY_DIR}/onnxruntime-linux-aarch64-static_lib-1.16.0/lib/aarch64)
elseif(AX_TTS_THIRDPARTY_FLAVOR STREQUAL "arm_uclibc")
    set(ORT_LIB_DIR ${THIRDPARTY_DIR}/onnxruntime-linux-aarch64-static_lib-1.16.0/lib/arm_uclibc)
elseif(AX_TTS_THIRDPARTY_FLAVOR STREQUAL "arm_gnueabihf")
    set(ORT_LIB_DIR ${THIRDPARTY_DIR}/onnxruntime-linux-aarch64-static_lib-1.16.0/lib/arm_gnueabihf)
else()
    message(FATAL_ERROR "Unknown third-party flavor: ${AX_TTS_THIRDPARTY_FLAVOR}")
endif()
if(NOT EXISTS ${ORT_LIB_DIR})
    message(FATAL_ERROR "onnxruntime lib dir not found: ${ORT_LIB_DIR}. Run download_bsp.sh first.")
endif()
list(APPEND ORT_LIBS onnxruntime)

include_directories(${ORT_INC_DIR})
link_directories(${ORT_LIB_DIR})
