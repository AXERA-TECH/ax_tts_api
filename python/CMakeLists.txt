cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

find_package(pybind11 REQUIRED)

pybind11_add_module(_ax_tts_core src/bindings.cpp)

target_include_directories(_ax_tts_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/api
    ${CMAKE_SOURCE_DIR}/src/utils
    ${MSP_INC_DIR}
    ${ESPEAK_INC_DIR}
    ${ORT_INC_DIR}
)

target_link_libraries(_ax_tts_core PRIVATE
    ax_tts_api
    ${MSP_LIBS}
    ${ESPEAK_LIBS}
    ${ORT_LIBS}
    dl
)

install(TARGETS _ax_tts_core
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
)
