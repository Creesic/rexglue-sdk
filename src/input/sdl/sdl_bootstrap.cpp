#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include <rex/input/sdl/sdl_bootstrap.h>

namespace rex::input::sdl {

void PrepareSDLForCustomMain() { SDL_SetMainReady(); }

}  // namespace rex::input::sdl
