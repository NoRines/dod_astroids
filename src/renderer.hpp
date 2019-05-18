#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SDL2/SDL.h>
#include <cstdint>

class renderer
{
public:
    static int init(const char* title, int width, int height)
    {
        _windowWidth = width;
        _windowHeight = height;

        SDL_Init(SDL_INIT_EVERYTHING);

        if(_window == nullptr && _renderer == nullptr)
        {
            _window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _windowWidth, _windowHeight, 0);
            _renderer = SDL_CreateRenderer(_window, -1, 0);
        }

        return 0;
    }

    static void quit()
    {
        SDL_DestroyRenderer(_renderer);
        SDL_DestroyWindow(_window);

        SDL_Quit();
    }

    static void clear()
    {
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);
    }

    static void drawLine(int x0, int y0, int x1, int y1, uint32_t color)
    {
        SDL_SetRenderDrawColor(_renderer, (uint8_t)(color >> 24), (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
        SDL_RenderDrawLine(_renderer, x0, y0, x1, y1);
    }

    static void show()
    {
        SDL_RenderPresent(_renderer);
    }

private:
    static SDL_Window* _window;
    static SDL_Renderer* _renderer;
    static int _windowWidth;
    static int _windowHeight;
};

SDL_Window* renderer::_window = nullptr;
SDL_Renderer* renderer::_renderer = nullptr;
int renderer::_windowWidth = 0;
int renderer::_windowHeight = 0;

#endif
