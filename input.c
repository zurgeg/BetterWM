#include "input.h"

#include "SDL/SDL.h"
#include <math.h>

/* This old file badly needs refactoring. */

int up, down, left, right;
bool steerright, steerleft;
double steerang = (PI / 2);
int arrow_function = 1; //0 for IR, 1 for nunchuck stick, 2 for classic stick
bool togglekey0 = 0;
bool togglekey9 = 0;
bool shift;

void input_init()
{
  //init SDL
  if (SDL_Init(0) < 0)
  {
    printf("Could not initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }
  SDL_SetVideoMode(128, 128, 0, 0);
}

void input_unload()
{
  SDL_Quit();
}

void input_update(struct wiimote_state * state)
{
  SDL_Event event;

  /* Loop through waiting messages and process them */

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            exit(0);
              break;

                    case SDLK_0:
                        if (togglekey0 == 0)
                        {
                            togglekey0 = 1;
                            arrow_function = (arrow_function + 1) % 4;
                            printf("arrows (IR, nunchuck, classic, wmp): %d \n", arrow_function);
                            if (arrow_function != 0)
                            {
                                  ir_object_clear(state, 0);
                                  ir_object_clear(state, 1);
                                  ir_object_clear(state, 2);
                                  ir_object_clear(state, 3);
                            } else {
                                  state->usr.ir_object[0].x = 400;
                                  state->usr.ir_object[0].y = 400;
                                  state->usr.ir_object[0].size = 8;
                                  state->usr.ir_object[1].x = 600;
                                  state->usr.ir_object[1].y = 400;
                                  state->usr.ir_object[1].size = 8;
                            }
                        }
                        break;
                    case SDLK_9:
                        if (togglekey9 == 0)
                        {
                            togglekey9 = 1;
                            show_reports = (show_reports + 1) % 2;
                        }
                        break;
                    case SDLK_LSHIFT:
                        shift = 1; break;
                    case SDLK_a:
                      state->usr.a = 1;
                        //state->usr.classic.a = 1;
                        break;
                    case SDLK_d:
                      state->usr.b = 1;
                        //state->usr.classic.b = 1;
                        break;
                    case SDLK_q:
                      state->usr.nunchuck.c = 1;
                      state->usr.classic.x = 1;
                        break;
                    case SDLK_e:
                      state->usr.nunchuck.z = 1;
                      state->usr.classic.y = 1;
                        break;
                    case SDLK_1:
                      state->usr.one = 1;
                        break;
                    case SDLK_2:
                      state->usr.two = 1;
                        break;
                    case SDLK_3:
                      state->usr.minus = 1;
                        break;
                    case SDLK_4:
                      state->usr.plus = 1;
                        break;
                    case SDLK_h:
                      state->usr.home = 1;
                        break;
                    case SDLK_KP8:
                        state->usr.up = 1;
                        break;
                    case SDLK_KP2:
                        state->usr.down = 1;
                        break;
                    case SDLK_KP4:
                        state->usr.left = 1;
                        break;
                    case SDLK_KP6:
                        state->usr.right = 1;
                        break;
          case SDLK_UP:
                        up = 1;
            break;

          case SDLK_DOWN:
                        down = 1;
            break;

          case SDLK_LEFT:
                        left = 1;
            break;

          case SDLK_RIGHT:
                        right = 1;
            break;

                    case SDLK_t:
                        steerleft = 1;
                        break;
                    case SDLK_y:
                        steerright = 1;
                        break;
                    default:
                        break;
                }
          break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                    case SDLK_0:
                        togglekey0 = 0;
                        break;
                    case SDLK_9:
                        togglekey9 = 0;
                        break;
                    case SDLK_LSHIFT:
                        shift = 0; break;
                    case SDLK_a:
                      state->usr.a = 0;
                        state->usr.classic.a = 0;
                        break;
                    case SDLK_d:
                      state->usr.b = 0;
                        state->usr.classic.b = 0;
                        break;
                    case SDLK_q:
                      state->usr.nunchuck.c = 0;
                        state->usr.classic.x = 0;
                        break;
                    case SDLK_e:
                      state->usr.nunchuck.z = 0;
                      state->usr.classic.y = 0;
                        break;
                    case SDLK_1:
                      state->usr.one = 0;
                        break;
                    case SDLK_2:
                      state->usr.two = 0;
                        break;
                    case SDLK_3:
                      state->usr.minus = 0;
                        break;
                    case SDLK_4:
                      state->usr.plus = 0;
                        break;
                    case SDLK_h:
                      state->usr.home = 0;
                        break;
                    case SDLK_KP8:
                        state->usr.up = 0;
                        break;
                    case SDLK_KP2:
                        state->usr.down = 0;
                        break;
                    case SDLK_KP4:
                        state->usr.left = 0;
                        break;
                    case SDLK_KP6:
                        state->usr.right = 0;
                        break;
                    case SDLK_UP:
                        up = 0;
            break;

          case SDLK_DOWN:
                        down = 0;
            break;

          case SDLK_LEFT:
                        left = 0;
            break;

          case SDLK_RIGHT:
                        right = 0;
            break;

                    case SDLK_t:
                        steerleft = 0;
                        break;
                    case SDLK_y:
                        steerright = 0;
                        break;
          default:
              break;
                }
    }
  }

    if ((steerleft && steerright) || (!steerleft && !steerright))
    {
        steerang = (PI / 2);
    } else if (steerleft)
    {
        steerang = (6 * PI / 8);
    } else if (steerright)
    {
        steerang = (2 * PI / 8);
    }

    /*

    if (steerleft)
    {
        if (steerang < (7 * PI / 8))
            steerang += 0.02;
        state->usr.accel_y = -cos(steerang) * (0x19 << 2) + 0x200;
        state->usr.accel_x = -sin(steerang) * (0x19 << 2) + 0x200;
    }

    if (steerright)
    {
        if (steerang > (1 * PI / 8))
            steerang -= 0.02;
        state->usr.accel_y = -cos(steerang) * (0x19 << 2) + 0x200;
        state->usr.accel_x = -sin(steerang) * (0x19 << 2) + 0x200;
    }

    */

    //state->usr.accel_y = -cos(steerang) * (0x19 << 2) + 0x200;
    //state->usr.accel_x = -sin(steerang) * (0x19 << 2) + 0x200;

    switch (arrow_function)
    {
        case 0:
            if (down)
            {
                 if (state->usr.ir_object[0].y < 764)
               {
                  state->usr.ir_object[0].y += 4;
                  state->usr.ir_object[1].y += 4;
               }
            }

            if (up)
            {
                 if (state->usr.ir_object[0].x > 3)
             {
                state->usr.ir_object[0].y -= 4;
                state->usr.ir_object[1].y -= 4;
             }

            }

            if (left)
            {
                 if (state->usr.ir_object[0].x < 1020)
             {
                 state->usr.ir_object[0].x += 4;
                 state->usr.ir_object[1].x += 4;
             }

            }

            if (right)
            {
                 if (state->usr.ir_object[0].x > 3)
                 {
                    state->usr.ir_object[0].x -= 4;
                    state->usr.ir_object[1].x -= 4;
               }
            }
            break;
        case 1:
            state->usr.nunchuck.x = 128 + right * 100 - left * 100;
            state->usr.nunchuck.y = 128 + up * 100 - down * 100;
            break;
        case 2:
            state->usr.classic.ls_x = 32 + right * 30 - left * 30;
            state->usr.classic.ls_y = 32 + up * 30 - down * 30;
            break;
        case 3:
            state->usr.motionplus.pitch_left = 0x1F7F + down * 800 * (1 + shift) - up * 800 * (1 + shift);
            state->usr.motionplus.yaw_down = 0x1F7F + left * 800 * (1 + shift) - right * 800 * (1 + shift);
            state->usr.motionplus.pitch_slow = !shift;
            state->usr.motionplus.yaw_slow = !shift;
            break;
    }
}
