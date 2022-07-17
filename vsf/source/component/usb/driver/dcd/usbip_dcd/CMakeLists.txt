# CMakeLists head

target_sources(${VSF_LIB_NAME} INTERFACE
    vsf_usbip_dcd.c
)
add_subdirectory(backend)
