/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <SDL.h>

#include "common.h"

#include "image.h"
#include "palette.h"
#include "video.h"
#include "event.h"
#include "timing.h"
#include "sprite.h"
#include "game.h"
#include "setup.h"

extern flags_struct flags;
extern int get_key_binding(char const *dir, int i);
extern int mouse_xscale, mouse_yscale;
short mouse_buttons[5] = { 0, 0, 0, 0, 0 };

#if (defined(__wii__) || defined(__gamecube__))
// need to track joystick axis states across calls to event handler in order
//  to simulate EV_KEYRELEASE events on return to center
bool joyaxisstates[4] = { false, false, false, false }; // left, right, up, down

// need to track D-pad buttons in order to merge up+left and down+right
bool joyhatstates[4] = { false, false }; // up, down

#endif

void EventHandler::SysInit()
{
    // Ignore activate events
    SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
}

void EventHandler::SysWarpMouse(ivec2 pos)
{
    SDL_WarpMouse(pos.x, pos.y);
}

//
// IsPending()
// Are there any events in the queue?
//
int EventHandler::IsPending()
{
    if (!m_pending && SDL_PollEvent(NULL))
        m_pending = 1;

    return m_pending;
}

//
// Get and handle waiting events
//
void EventHandler::SysEvent(Event &ev)
{
    // No more events
    m_pending = 0;

    // NOTE : that the mouse status should be known
    // even if another event has occurred.
    ev.mouse_move.x = m_pos.x;
    ev.mouse_move.y = m_pos.y;
    ev.mouse_button = m_button;

    // Gather next event
    SDL_Event sdlev;
    if (!SDL_PollEvent(&sdlev))
        return; // This should not happen

    // Sort the mouse out
    int x, y;
#if (defined(__wii__) || defined(__gamecube__))
    // Don't care about button states on the Wii since they don't exist
    // (eliminates compiler warning for unused buttons variable)
    SDL_GetMouseState(&x, &y);
#else
    uint8_t buttons = SDL_GetMouseState(&x, &y);
#endif
    x = Min((x << 16) / mouse_xscale, main_screen->Size().x - 1);
    y = Min((y << 16) / mouse_yscale, main_screen->Size().y - 1);
    ev.mouse_move.x = x;
    ev.mouse_move.y = y;
    ev.type = EV_MOUSE_MOVE;

#if (defined(__wii__) || defined(__gamecube__))
    // On Wii, skip mouse button processing and simulate mouse buttons via
    //  joystick button events. Do nothing here.
    //
    // Also, why is m_pos updated below instead of above?
#else
    // Left button
    if((buttons & SDL_BUTTON(1)) && !mouse_buttons[1])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[1] = !mouse_buttons[1];
        ev.mouse_button |= LEFT_BUTTON;
    }
    else if(!(buttons & SDL_BUTTON(1)) && mouse_buttons[1])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[1] = !mouse_buttons[1];
        ev.mouse_button &= (0xff - LEFT_BUTTON);
    }

    // Middle button
    if((buttons & SDL_BUTTON(2)) && !mouse_buttons[2])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[2] = !mouse_buttons[2];
        ev.mouse_button |= LEFT_BUTTON;
        ev.mouse_button |= RIGHT_BUTTON;
    }
    else if(!(buttons & SDL_BUTTON(2)) && mouse_buttons[2])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[2] = !mouse_buttons[2];
        ev.mouse_button &= (0xff - LEFT_BUTTON);
        ev.mouse_button &= (0xff - RIGHT_BUTTON);
    }

    // Right button
    if((buttons & SDL_BUTTON(3)) && !mouse_buttons[3])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[3] = !mouse_buttons[3];
        ev.mouse_button |= RIGHT_BUTTON;
    }
    else if(!(buttons & SDL_BUTTON(3)) && mouse_buttons[3])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[3] = !mouse_buttons[3];
        ev.mouse_button &= (0xff - RIGHT_BUTTON);
    }
#endif

    m_pos = ivec2(ev.mouse_move.x, ev.mouse_move.y);

#if (defined(__wii__) || defined(__gamecube__))
    // On Wii, skip mouse button processing and simulate mouse buttons via
    //  joystick button events. Do nothing here.
#else
    m_button = ev.mouse_button;
#endif

    // Sort out other kinds of events
    switch(sdlev.type)
    {
        case SDL_QUIT:
            exit(0);
            break;

#if (defined(__wii__) || defined(__gamecube__))
        case SDL_JOYAXISMOTION:
            // default to EV_SPURIOUS in case user moves joystick without
            //  changing active zone (e.g. left->left)
            ev.key = EV_SPURIOUS;

            if (sdlev.jaxis.axis == 0)                        // horizontal axis
            {
                if (sdlev.jaxis.value < -flags.hdeadzone)     // left
                {
                    if (!joyaxisstates[0])
                    {
                        ev.type = EV_KEY;
                        ev.key = JK_LEFT;
                        joyaxisstates[0] = true;
                    }
                    else if (joyaxisstates[1])
                    {
                        // it's possible to move the stick to the other side so
                        //  fast that SDL misses the center point, so attempt
                        //  to catch this case while the stick moves around in
                        //  the new zone
                        ev.type = EV_KEYRELEASE;
                        ev.key = JK_RIGHT;
                        joyaxisstates[1] = false;
                    }
                }
                else if (sdlev.jaxis.value > flags.hdeadzone) // right
                {
                    if (!joyaxisstates[1])
                    {
                        ev.type = EV_KEY;
                        ev.key = JK_RIGHT;
                        joyaxisstates[1] = true;
                    }
                    else if (joyaxisstates[0])
                    {
                        // it's possible to move the stick to the other side so
                        //  fast that SDL misses the center point, so attempt
                        //  to catch this case while the stick moves around in
                        //  the new zone
                        ev.type = EV_KEYRELEASE;
                        ev.key = JK_LEFT;
                        joyaxisstates[0] = false;
                    }
                }
                else                                          // center
                {
                    ev.type = EV_KEYRELEASE;
                    if (joyaxisstates[0])
                    {
                        ev.key = JK_LEFT;
                        joyaxisstates[0] = false;
                    }
                    else if (joyaxisstates[1])
                    {
                        ev.key = JK_RIGHT;
                        joyaxisstates[1] = false;
                    }
                }
            }
            else if (sdlev.jaxis.axis == 1 && flags.usevaxis) // vertical axis
            {
                if (sdlev.jaxis.value < -flags.vdeadzone)     // up
                {
                    ev.type = EV_KEY;
                    ev.key = JK_UP;
                    joyaxisstates[2] = true;
                    joyaxisstates[3] = false;
                }
                else if (sdlev.jaxis.value > flags.vdeadzone) // down
                {
                    ev.type = EV_KEY;
                    ev.key = JK_DOWN;
                    joyaxisstates[2] = false;
                    joyaxisstates[3] = true;
                }
                else                                          // center
                {
                    ev.type = EV_KEYRELEASE;
                    if (joyaxisstates[2])
                    {
                        ev.key = JK_UP;
                    }
                    else if (joyaxisstates[3])
                    {
                        ev.key = JK_DOWN;
                    }

                    joyaxisstates[2] = false;
                    joyaxisstates[3] = false;
                }
            }
            break;

        case SDL_JOYBUTTONDOWN:
            switch (sdlev.jbutton.button)
            {
                case  0: // wiimote A
                    // simulate right mouse button down
                    if (!mouse_buttons[3])
                    {
                        ev.type = EV_MOUSE_BUTTON;
                        mouse_buttons[3] = !mouse_buttons[3];
                        ev.mouse_button |= RIGHT_BUTTON;
                        m_button = ev.mouse_button;
                    }
                    break;

                case  1: // wiimote B
                    // simulate left mouse button down
                    if (!mouse_buttons[1])
                    {
                        ev.type = EV_MOUSE_BUTTON;
                        mouse_buttons[1] = !mouse_buttons[1];
                        ev.mouse_button |= LEFT_BUTTON;
                        m_button = ev.mouse_button;
                    }
                    break;

                case 2: // wiimote 1
                    // simulate spacebar down
                    ev.type = EV_KEY;
                    ev.key = JK_SPACE;
                    break;

                case 3: // wiimote 2
                    // simulate p key down
                    ev.type = EV_KEY;
                    ev.key = 'p';
                    break;

                case  4: // wiimote -
                case 17: // classic -
                    // simulate comma key (prev weapon) pressed
                    ev.type = EV_KEY;
                    ev.key = SDLK_COMMA;
                    break;

                case  5: // wiimote +
                case 18: // classic +
                    // simulate period key (next weapon) pressed
                    ev.type = EV_KEY;
                    ev.key = SDLK_PERIOD;
                    break;

                case  6: // wiimote home
                case 19: // classic home
                    ev.type = EV_KEY;
                    ev.key = JK_ESC;
                    break;

                case  7: // nunchuck Z
                case 15: // classic ZL (only one or the other can be connected)
                    ev.type = EV_KEY;
                    ev.key = (flags.swapbuttons ? JK_DOWN : JK_UP);
                    break;

                case  8: // nunchuck C
                case 13: // classic L (only one or the other can be connected)
                    ev.type = EV_KEY;
                    ev.key = (flags.swapbuttons ? JK_UP : JK_DOWN);
                    break;
            }
            break;

        case SDL_JOYBUTTONUP:
            switch (sdlev.jbutton.button)
            {
                case  0: // wiimote A
                    // simulate right mouse button up
                    if (mouse_buttons[3])
                    {
                        ev.type = EV_MOUSE_BUTTON;
                        mouse_buttons[3] = !mouse_buttons[3];
                        ev.mouse_button &= (0xff - RIGHT_BUTTON);
                        m_button = ev.mouse_button;
                    }
                    break;

                case  1: // wiimote B
                    // simulate left mouse button up
                    if(mouse_buttons[1])
                    {
                        ev.type = EV_MOUSE_BUTTON;
                        mouse_buttons[1] = !mouse_buttons[1];
                        ev.mouse_button &= (0xff - LEFT_BUTTON);
                        m_button = ev.mouse_button;
                    }
                    break;

                case 2: // wiimote 1
                    // simulate spacebar up
                    ev.type = EV_KEYRELEASE;
                    ev.key = JK_SPACE;
                    break;

                case 3: // wiimote 2
                    // simulate p key up
                    ev.type = EV_KEYRELEASE;
                    ev.key = 'p';
                    break;

                case  4: // wiimote -
                case 17: // classic -
                    // simulate comma key (prev weapon) released
                    ev.type = EV_KEYRELEASE;
                    ev.key = SDLK_COMMA;
                    break;

                case  5: // wiimote +
                case 18: // classic +
                    // simulate period key (next weapon) released
                    ev.type = EV_KEYRELEASE;
                    ev.key = SDLK_PERIOD;
                    break;

                case  6: // wiimote home
                case 19: // classic home
                    ev.type = EV_KEYRELEASE;
                    ev.key = JK_ESC;
                    break;

                case  7: // nunchuck Z
                case 15: // classic ZL (only one or the other can be connected)
                    ev.type = EV_KEYRELEASE;
                    ev.key = (flags.swapbuttons ? JK_DOWN : JK_UP);
                    break;

                case  8: // nunchuck C
                case 13: // classic L (only one or the other can be connected)
                    ev.type = EV_KEYRELEASE;
                    ev.key = (flags.swapbuttons ? JK_UP : JK_DOWN);
                    break;
            }
            break;

        case SDL_JOYHATMOTION:
            ev.key = EV_SPURIOUS;

            // check for release of up and left
            if (joyhatstates[0] &&
                (sdlev.jhat.value &
                 (SDL_HAT_CENTERED | SDL_HAT_RIGHT | SDL_HAT_DOWN)))
            {
                // simulate mousewheel up release
                ev.key = get_key_binding("b4", 0);
                ev.type = EV_KEYRELEASE;
                joyhatstates[0] = false;
            }
            // check for release of down and right
            else if (joyhatstates[1] &&
                     (sdlev.jhat.value &
                      (SDL_HAT_CENTERED | SDL_HAT_LEFT | SDL_HAT_UP)))
            {
                // simulate mousewheel down release
                ev.key = get_key_binding("b3", 0);
                ev.type = EV_KEYRELEASE;
                joyhatstates[1] = false;
            }
            // check for press of up and/or left
            else if (!joyhatstates[0] &&
                     (sdlev.jhat.value &
                      (SDL_HAT_LEFT | SDL_HAT_UP)))
            {
                // simulate mousewheel up press
                ev.key = get_key_binding("b4", 0);
                ev.type = EV_KEY;
                joyhatstates[0] = true;
            }
            // check for press of down and/or right
            else if (!joyhatstates[1] &&
                     (sdlev.jhat.value &
                      (SDL_HAT_RIGHT | SDL_HAT_DOWN)))
            {
                // simulate mousewheel down press
                ev.key = get_key_binding("b3", 0);
                ev.type = EV_KEY;
                joyhatstates[1] = true;
            }
            break;
#else
        case SDL_MOUSEBUTTONUP:
            switch(sdlev.button.button)
            {
            case 4:        // Mouse wheel goes up...
                ev.key = get_key_binding("b4", 0);
                ev.type = EV_KEYRELEASE;
                break;
            case 5:        // Mouse wheel goes down...
                ev.key = get_key_binding("b3", 0);
                ev.type = EV_KEYRELEASE;
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            switch(sdlev.button.button)
            {
            case 4:        // Mouse wheel goes up...
                ev.key = get_key_binding("b4", 0);
                ev.type = EV_KEY;
                break;
            case 5:        // Mouse wheel goes down...
                ev.key = get_key_binding("b3", 0);
                ev.type = EV_KEY;
                break;
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            // Default to EV_SPURIOUS
            ev.key = EV_SPURIOUS;
            if(sdlev.type == SDL_KEYDOWN)
            {
                ev.type = EV_KEY;
            }
            else
            {
                ev.type = EV_KEYRELEASE;
            }
            switch(sdlev.key.keysym.sym)
            {
                case SDLK_DOWN:         ev.key = JK_DOWN; break;
                case SDLK_UP:           ev.key = JK_UP; break;
                case SDLK_LEFT:         ev.key = JK_LEFT; break;
                case SDLK_RIGHT:        ev.key = JK_RIGHT; break;
                case SDLK_LCTRL:        ev.key = JK_CTRL_L; break;
                case SDLK_RCTRL:        ev.key = JK_CTRL_R; break;
                case SDLK_LALT:         ev.key = JK_ALT_L; break;
                case SDLK_RALT:         ev.key = JK_ALT_R; break;
                case SDLK_LSHIFT:       ev.key = JK_SHIFT_L; break;
                case SDLK_RSHIFT:       ev.key = JK_SHIFT_R; break;
                case SDLK_NUMLOCK:      ev.key = JK_NUM_LOCK; break;
                case SDLK_HOME:         ev.key = JK_HOME; break;
                case SDLK_END:          ev.key = JK_END; break;
                case SDLK_BACKSPACE:    ev.key = JK_BACKSPACE; break;
                case SDLK_TAB:          ev.key = JK_TAB; break;
                case SDLK_RETURN:       ev.key = JK_ENTER; break;
                case SDLK_SPACE:        ev.key = JK_SPACE; break;
                case SDLK_CAPSLOCK:     ev.key = JK_CAPS; break;
                case SDLK_ESCAPE:       ev.key = JK_ESC; break;
                case SDLK_F1:           ev.key = JK_F1; break;
                case SDLK_F2:           ev.key = JK_F2; break;
                case SDLK_F3:           ev.key = JK_F3; break;
                case SDLK_F4:           ev.key = JK_F4; break;
                case SDLK_F5:           ev.key = JK_F5; break;
                case SDLK_F6:           ev.key = JK_F6; break;
                case SDLK_F7:           ev.key = JK_F7; break;
                case SDLK_F8:           ev.key = JK_F8; break;
                case SDLK_F9:           ev.key = JK_F9; break;
                case SDLK_F10:          ev.key = JK_F10; break;
                case SDLK_INSERT:       ev.key = JK_INSERT; break;
                case SDLK_KP0:          ev.key = JK_INSERT; break;
                case SDLK_PAGEUP:       ev.key = JK_PAGEUP; break;
                case SDLK_PAGEDOWN:     ev.key = JK_PAGEDOWN; break;
                case SDLK_KP8:          ev.key = JK_UP; break;
                case SDLK_KP2:          ev.key = JK_DOWN; break;
                case SDLK_KP4:          ev.key = JK_LEFT; break;
                case SDLK_KP6:          ev.key = JK_RIGHT; break;
                case SDLK_F11:
                    // Only handle key down
                    if(ev.type == EV_KEY)
                    {
                        // Toggle fullscreen
                        SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
                    }
                    ev.key = EV_SPURIOUS;
                    break;
                case SDLK_F12:
                    // Only handle key down
                    if(ev.type == EV_KEY)
                    {
                        // Toggle grab mouse
                        if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
                        {
                            the_game->show_help("Grab Mouse: OFF\n");
                            SDL_WM_GrabInput(SDL_GRAB_OFF);
                        }
                        else
                        {
                            the_game->show_help("Grab Mouse: ON\n");
                            SDL_WM_GrabInput(SDL_GRAB_ON);
                        }
                    }
                    ev.key = EV_SPURIOUS;
                    break;
                case SDLK_PRINT:    // print-screen key
                    // Only handle key down
                    if(ev.type == EV_KEY)
                    {
                        // Grab a screenshot
                        SDL_SaveBMP(SDL_GetVideoSurface(), "screenshot.bmp");
                        the_game->show_help("Screenshot saved to: screenshot.bmp.\n");
                    }
                    ev.key = EV_SPURIOUS;
                    break;
                default:
                    ev.key = (int)sdlev.key.keysym.sym;
                    // Need to handle the case of shift being pressed
                    // There has to be a better way
                    if((sdlev.key.keysym.mod & KMOD_SHIFT) != 0)
                    {
                        if(sdlev.key.keysym.sym >= SDLK_a &&
                            sdlev.key.keysym.sym <= SDLK_z)
                        {
                            ev.key -= 32;
                        }
                        else if(sdlev.key.keysym.sym >= SDLK_1 &&
                                 sdlev.key.keysym.sym <= SDLK_5)
                        {
                            ev.key -= 16;
                        }
                        else
                        {
                            switch(sdlev.key.keysym.sym)
                            {
                                case SDLK_6:
                                    ev.key = SDLK_CARET; break;
                                case SDLK_7:
                                case SDLK_9:
                                case SDLK_0:
                                    ev.key -= 17; break;
                                case SDLK_8:
                                    ev.key = SDLK_ASTERISK; break;
                                case SDLK_MINUS:
                                    ev.key = SDLK_UNDERSCORE; break;
                                case SDLK_EQUALS:
                                    ev.key = SDLK_PLUS; break;
                                case SDLK_COMMA:
                                    ev.key = SDLK_LESS; break;
                                case SDLK_PERIOD:
                                    ev.key = SDLK_GREATER; break;
                                case SDLK_SLASH:
                                    ev.key = SDLK_QUESTION; break;
                                case SDLK_SEMICOLON:
                                    ev.key = SDLK_COLON; break;
                                case SDLK_QUOTE:
                                    ev.key = SDLK_QUOTEDBL; break;
                                default:
                                    break;
                            }
                        }
                    }
                    break;
            }
#endif
    }
}

