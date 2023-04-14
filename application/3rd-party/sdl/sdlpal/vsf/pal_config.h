#define PAL_PREFIX                      "./"
#define PAL_SAVE_PREFIX                 "./"

#define PAL_DEFAULT_WINDOW_WIDTH        320
#define PAL_DEFAULT_WINDOW_HEIGHT       200
#define PAL_DEFAULT_FULLSCREEN_HEIGHT   200

#if SDL_VERSION_ATLEAST(2,0,0)
# define PAL_VIDEO_INIT_FLAGS           (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (gConfig.fFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))
#else
# define PAL_VIDEO_INIT_FLAGS           (SDL_HWSURFACE | SDL_RESIZABLE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# define PAL_FATAL_OUTPUT(s)            vsf_trace_error(s)
# define PAL_HAS_SDLCD                  1
#endif

#if VSF_USE_AUDIO == ENABLED
#   define PAL_SDL_INIT_FLAGS           (SDL_INIT_VIDEO | SDL_INIT_AUDIO)
#else
#   define PAL_SDL_INIT_FLAGS           SDL_INIT_VIDEO
#endif
#define PAL_SDL_RENDERER_FLAGS          0

#define PAL_PLATFORM                    NULL
#define PAL_CREDIT                      NULL
#define PAL_PORTYEAR                    NULL

#define PAL_PLATFORM                    NULL
#define PAL_CREDIT                      NULL
#define PAL_PORTYEAR                    NULL

#define PAL_HAS_NATIVEMIDI              0

#define PAL_HAS_MP3                     0
#define PAL_HAS_OGG                     0
#define PAL_HAS_OPUS                    0
#define PAL_HAS_SDLCD                   0
