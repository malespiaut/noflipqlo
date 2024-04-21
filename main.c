#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>

int past_m = 0;

bool done = false;
bool twentyfourh = true;
bool fullscreen = false;

TTF_Font* font_time_g = NULL;
TTF_Font* font_mode_g = NULL;
const SDL_Color font_color_white_k = {.r = 176, .g = 176, .b = 176, .a = 0};

SDL_FRect background_hours = {0};
SDL_FRect background_minutes = {0};
int spacing = 0;

int screen_height_custom = 0;
int screen_width_custom = 0;

#define screen_width_k 640
#define screen_height_k 480

#define font_bold_path_k "/usr/share/fonts/droid/DroidSans-Bold.ttf"
#define font_fallback_path_k "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-B.ttf"
const SDL_Color font_color_k = {.r = 183, .g = 183, .b = 183, .a = 255};
const SDL_Color background_color_k = {.r = 10, .g = 10, .b = 10, .a = 255};

char* font_custom_path_g = "";

SDL_Window* window_g = NULL;
SDL_Renderer* renderer_g = NULL;

static int
resources_init(void)
{
  if (strcmp("", font_custom_path_g) == 0)
  {
    font_time_g = TTF_OpenFont(font_bold_path_k, screen_height_custom / 2);
    font_mode_g = TTF_OpenFont(font_bold_path_k, screen_height_custom / 15);
  }
  else
  {
    font_time_g = TTF_OpenFont(font_custom_path_g, screen_height_custom / 2);
    font_time_g = TTF_OpenFont(font_custom_path_g, screen_height_custom / 15);
  }

  /* CALCULATE BACKGROUND COORDINATES */
  background_hours.y = 0.2f * screen_height_custom;
  background_hours.x = 0.5f * (screen_width_custom - ((0.031f) * screen_width_custom) - (1.2f * screen_height_custom));
  background_hours.w = screen_height_custom * 0.6f;
  background_hours.h = background_hours.w;

  spacing = 0.031f * screen_width_custom;

  background_minutes.x = background_hours.x + (0.6f * screen_height_custom) + spacing;
  background_minutes.y = background_hours.y;
  background_minutes.h = background_hours.h;
  background_minutes.w = background_hours.w;
  return 0;
}

static void
resources_deinit(void)
{
  TTF_CloseFont(font_time_g);
  TTF_CloseFont(font_mode_g);
}

static void
circle_fill(SDL_FPoint p, float radius, SDL_Color c)
{
  SDL_SetRenderDrawColor(renderer_g, c.r, c.g, c.b, c.a);
  for (float dy = 1.0f; dy <= radius; dy += 1.0f)
  {
    float dx = floorf(sqrtf((2.0f * radius * dy) - (dy * dy)));
    SDL_RenderDrawLineF(renderer_g, p.x - dx, p.y + dy - radius, p.x + dx, p.y + dy - radius);
    SDL_RenderDrawLineF(renderer_g, p.x - dx, p.y - dy + radius, p.x + dx, p.y - dy + radius);
  }
}

static void
background_rounded_draw(const SDL_FRect* coordinates)
{
  int background_size = screen_height_custom * 0.6f;
  float radius = 10.0f;
  for (int i = 0; i < background_size - radius; i++)
  {
    for (int j = 0; j < background_size - radius; j++)
    {
      circle_fill((SDL_FPoint){.x = coordinates->x + j, .y = coordinates->y + i}, radius, (SDL_Color){.r = 10, .g = 10, .b = 10, .a = 255});
    }
  }
}

static SDL_FPoint
coordinates_get(const SDL_FRect* background, const SDL_Surface* foreground)
{
  int dx = (background->w - foreground->w) * 0.5f;
  int dy = (background->h - foreground->h) * 0.5f;

  return (SDL_FPoint){
    .x = background->x + dx,
    .y = background->y + dy,
  };
}

static void
divider_draw(void)
{
  const SDL_FRect line = {
    .h = screen_height_custom * 0.0051f,
    .w = screen_width_custom,
    .x = 0,
    .y = (screen_height_custom * 0.5f) - (screen_height_custom * 0.0051f),
  };

  SDL_SetRenderDrawColor(renderer_g, 0, 0, 0, 255);
  SDL_RenderFillRectF(renderer_g, &line);
}

static void
ampm_draw(const struct tm* _time)
{
  SDL_FPoint coordinates = {
    .x = (background_hours.h * 0.024f) + background_hours.x,
    .y = (background_hours.h * 0.071f) + background_hours.y,
  };
  char mode[3] = {0};
  strftime(mode, 3, "%p", _time);

  SDL_Surface* ampm = TTF_RenderText_Blended(font_mode_g, (const char*)mode, font_color_white_k);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_g, ampm);
  SDL_FreeSurface(ampm);
  SDL_FRect ampm_dstrect = {.x = coordinates.x, .y = coordinates.y, .w = ampm->w, .h = ampm->h};
  SDL_RenderCopyF(renderer_g, texture, NULL, &ampm_dstrect);
  SDL_DestroyTexture(texture);
}

static void
time_draw(const struct tm* _time)
{
  char hour[3] = {0};
  if (twentyfourh)
  {
    strftime(hour, 3, "%H", _time);
  }
  else
  {
    strftime(hour, 3, "%I", _time);
  }
  char minutes[3] = {0};
  strftime(minutes, 3, "%M", _time);

  int h = atoi(hour);
  char buff[2] = {0};
  sprintf(buff, "%d", h);

  SDL_Surface* text = TTF_RenderText_Blended(font_time_g, buff, font_color_white_k);
  SDL_FPoint coordinates = coordinates_get(&background_hours, text);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_g, text);
  SDL_FreeSurface(text);
  SDL_FRect hours_dstrect = {.x = coordinates.x, .y = coordinates.y, .w = text->w, .h = text->h};
  SDL_RenderCopyF(renderer_g, texture, NULL, &hours_dstrect);
  SDL_DestroyTexture(texture);

  text = TTF_RenderText_Blended(font_time_g, (const char*)minutes, font_color_white_k);
  coordinates = coordinates_get(&background_minutes, text);
  texture = SDL_CreateTextureFromSurface(renderer_g, text);
  SDL_FreeSurface(text);
  SDL_FRect minutes_dstrect = {.x = coordinates.x, .y = coordinates.y, .w = text->w, .h = text->h};
  SDL_RenderCopyF(renderer_g, texture, NULL, &minutes_dstrect);
  SDL_DestroyTexture(texture);
}

static void
draw(void)
{
  SDL_SetRenderDrawColor(renderer_g, 0, 0, 0, 255);
  SDL_RenderClear(renderer_g);

  time_t rawTime = {0};
  time(&rawTime);
  const struct tm* _time = localtime(&rawTime);

  background_rounded_draw(&background_hours);
  background_rounded_draw(&background_minutes);

  time_draw(_time);

  if (!twentyfourh)
  {
    ampm_draw(_time);
  }

  divider_draw();

  SDL_RenderPresent(renderer_g);
}

static void
sdl_init(void)
{
  window_g = SDL_CreateWindow("noflipqlo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width_k, screen_height_k, SDL_WINDOW_SHOWN);
  renderer_g = SDL_CreateRenderer(window_g, -1, SDL_RENDERER_PRESENTVSYNC);
  SDL_RenderSetLogicalSize(renderer_g, screen_width_k, screen_height_k);

  TTF_Init();
}

static void
sdl_deinit(void)
{
  SDL_DestroyRenderer(renderer_g);
  SDL_DestroyWindow(window_g);
  SDL_Quit();

  TTF_Quit();
}

static void
events_process(void)
{
  SDL_Event event = {0};
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE)
          done = true;
        break;
      case SDL_QUIT:
        done = true;
        break;
    }
  }
}

int
main(int argc, char** argv)
{
  static char sdlwid[100] = {0};
  /* This variable MUST stay uninitialised for the X11 window to work */
  unsigned int wid;
  Display* display = NULL;
  XWindowAttributes windowAttributes = {0};

  /* If no window argument, check environment */
  char* wid_env = NULL;
  if (!wid)
  {
    wid_env = getenv("XSCREENSAVER_WINDOW");
    if (wid_env)
    {
      wid = strtol(wid_env, (char**)NULL, 0); /* Base 0 autodetects hex/dec */
    }
  }

  /* Get win attrs if we've been given a window, otherwise we'll use our own */
  if (!wid)
  {
    display = XOpenDisplay(NULL);
    if (display)
    { /* Use the default display */
      XGetWindowAttributes(display, (Window)wid, &windowAttributes);
      XCloseDisplay(display);
      snprintf(sdlwid, 100, "SDL_WINDOWID=0x%X", wid);
      putenv(sdlwid); /* Tell SDL to use this window */
    }
  }

  for (int i = 1; i < argc; i++)
  {
    if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0)
    {
      printf("Usage: [OPTION...]\nOptions:\n");
      printf(" --help\t\t\t\tDisplay this\n");
      printf(" -root,--fullscreen,--root\tFullscreen\n");
      printf(" -ampm, --ampm\t\t\tTurn off 24 h system and use 12 h system instead\n");
      printf(" -w\t\t\t\tCustom Width\n");
      printf(" -h\t\t\t\tCustom Height\n");
      printf(" -r, --resolution\t\tCustom resolution in a format [Width]x[Height]\n");
      printf(" -f, --font\t\t\tPath to custom file font. Has to be Truetype font.");
      return 0;
    }
    else if (strcmp("-root", argv[i]) == 0 || strcmp("--root", argv[i]) == 0 || strcmp("--fullscreen", argv[i]) == 0)
    {
      fullscreen = true;
    }
    else if (strcmp("-ampm", argv[i]) == 0 || strcmp("--ampm", argv[i]) == 0)
    {
      twentyfourh = false;
    }
    else if (strcmp("-w", argv[i]) == 0)
    {
      screen_width_custom = atoi(argv[i + 1]);
    }
    else if (strcmp("-h", argv[i]) == 0)
    {
      screen_height_custom = atoi(argv[i + 1]);
    }
    else if (strcmp("-f", argv[i]) == 0 || strcmp("--font", argv[i]) == 0)
    {
      font_custom_path_g = argv[i + 1];
    }
    else
    {
      printf("Invalid option -- %s\n", argv[i]);
      printf("Try --help for more information.\n");
      return 0;
    }
  }

  if (screen_height_custom <= 0)
  {
    screen_height_custom = screen_height_k;
    screen_width_custom = screen_width_k;
  }

  sdl_init();
  SDL_ShowCursor(SDL_DISABLE);
  resources_init();

  if (fullscreen)
  {
    SDL_SetWindowFullscreen(window_g, SDL_WINDOW_FULLSCREEN_DESKTOP);

    screen_height_custom = screen_height_k;
    screen_width_custom = screen_width_k;
  }

  while (!done)
  {
    events_process();
    draw();
  }
  resources_deinit();
  sdl_deinit();
  return 0;
}
