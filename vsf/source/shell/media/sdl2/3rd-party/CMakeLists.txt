# CMakeLists head

if(VSF_SDL2_USE_TTF)
    target_include_directories(${VSF_TARGET_NAME} PUBLIC
        SDL_ttf
    )
    vsf_add_sources(
        SDL_ttf/SDL_ttf.c
    )
endif()

if(VSF_SDL2_USE_IMAGE)
    target_include_directories(${VSF_TARGET_NAME} PUBLIC
        SDL_image
    )
    vsf_add_sources(
        SDL_image/IMG.c
        SDL_image/IMG_bmp.c
        SDL_image/IMG_gif.c
        SDL_image/IMG_jpg.c
        SDL_image/IMG_lbm.c
        SDL_image/IMG_pcx.c
        SDL_image/IMG_png.c
        SDL_image/IMG_pnm.c
        SDL_image/IMG_svg.c
        SDL_image/IMG_tga.c
        SDL_image/IMG_tif.c
        SDL_image/IMG_webp.c
        SDL_image/IMG_WIC.c
        SDL_image/IMG_xcf.c
        SDL_image/IMG_xpm.c
        SDL_image/IMG_xv.c
        SDL_image/IMG_xxx.c
    )
endif()