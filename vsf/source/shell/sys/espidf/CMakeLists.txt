# CMakeLists head
#
# vsf_espidf - ESP-IDF compatible sub-system for VSF
#
# Layout mirrors shell/sys/linux:
#   include/           - clean-room public headers shaped as ESP-IDF API
#   port/              - glue code bridging ESP-IDF API to VSF internals
#   app/               - app_main entry shim (future)
#   vsf_espidf.c       - sub-system init + common helpers
#   vsf_espidf.h       - internal header
#   vsf_espidf_cfg.h   - feature switches

if(VSF_USE_ESPIDF)
    target_include_directories(${VSF_TARGET_NAME} BEFORE PUBLIC
        include
    )

    vsf_add_sources(
        vsf_espidf.c
    )

    add_subdirectory(port)
endif()
