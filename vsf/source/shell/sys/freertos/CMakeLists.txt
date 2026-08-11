# CMakeLists head
#
# vsf_freertos - FreeRTOS-compatible sub-system for VSF
#
# Layout mirrors shell/sys/espidf:
#   include/           - clean-room public headers shaped as FreeRTOS API
#   port/              - glue code bridging FreeRTOS API to VSF internals
#   vsf_freertos.c     - sub-system init + common helpers
#   vsf_freertos.h     - internal header
#   vsf_freertos_cfg.h - feature switches
#
# vsf_freertos is an independent sub-system and MUST NOT depend on
# shell/sys/espidf. The reverse is allowed: espidf may call into the
# FreeRTOS shim (e.g. vTaskDelay, xQueueCreate).

if(VSF_USE_FREERTOS)
    target_include_directories(${VSF_TARGET_NAME} BEFORE PUBLIC
        include
    )

    vsf_add_sources(
        vsf_freertos.c
    )

    add_subdirectory(port)
endif()
