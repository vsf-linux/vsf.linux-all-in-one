# CMakeLists head

target_sources(${VSF_LIB_NAME} INTERFACE
    vsf_hal_distbus.c
)

add_subdirectory(usbd)