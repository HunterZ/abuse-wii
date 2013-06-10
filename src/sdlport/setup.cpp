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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <SDL.h>
#ifdef HAVE_OPENGL
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif    /* __APPLE__ */
#endif    /* HAVE_OPENGL */

#if (defined(__wii__) || defined(__gamecube__))
#include <fat.h> // wii-specific filesystem library
#endif

#include "specs.h"
#include "keys.h"
#include "setup.h"

flags_struct flags;
keys_struct keys;

extern int xres, yres;
static unsigned int scale;

//
// Display help
//
void showHelp()
{
    printf( "\n" );
    printf( "Usage: abuse.sdl [options]\n" );
    printf( "Options:\n\n" );
    printf( "** Abuse Options **\n" );
    printf( "  -size <arg>       Set the size of the screen\n" );
    printf( "  -edit             Startup in editor mode\n" );
    printf( "  -a <arg>          Use addon named <arg>\n" );
    printf( "  -f <arg>          Load map file named <arg>\n" );
    printf( "  -lisp             Startup in lisp interpreter mode\n" );
    printf( "  -nodelay          Run at maximum speed\n" );
    printf( "\n" );
    printf( "** Abuse-SDL Options **\n" );
    printf( "  -datadir <arg>    Set the location of the game data to <arg>\n" );
    printf( "  -doublebuf        Enable double buffering\n" );
    printf( "  -fullscreen       Enable fullscreen mode\n" );
#ifdef HAVE_OPENGL
    printf( "  -gl               Enable OpenGL\n" );
    printf( "  -antialias        Enable anti-aliasing (with -gl only)\n" );
#endif
    printf( "  -h, --help        Display this text\n" );
    printf( "  -mono             Disable stereo sound\n" );
    printf( "  -nosound          Disable sound\n" );
    printf( "  -scale <arg>      Scale to <arg>\n" );
//    printf( "  -x <arg>          Set the width to <arg>\n" );
//    printf( "  -y <arg>          Set the height to <arg>\n" );
#if (defined(__wii__) || defined(__gamecube__))
    printf( "** abuse-wii Options **\n" );
    printf( "  -widestretch      Stretch picture horizontally when Wii is set for 16:9\n" );
    printf( "  -swapbuttons      Swap jump/activate/climb button controls\n" );
    printf( "  -usevaxis         Allow vertical axis to control jump/activate/climb\n" );
    printf( "  -hdeadzone <arg>  Set horizontal axis deadzone to <arg> percent\n" );
    printf( "  -vdeadzone <arg>  Set vertical axis deadzone to <arg> percent\n" );
#endif
    printf( "\n" );
    printf( "Anthony Kruize <trandor@labyrinth.net.au>\n" );
    printf( "\n" );
}

//
// Create a default 'abuserc' file
//
void createRCFile( char *rcfile )
{
#if (defined(__wii__) || defined(__gamecube__))
    // not useful for Wii
    return;
#else
    FILE *fd = NULL;

    if( (fd = fopen( rcfile, "w" )) != NULL )
    {
        fputs( "; Abuse-SDL Configuration file\n\n", fd );
        fputs( "; Startup fullscreen\nfullscreen=0\n\n", fd );
#ifdef __APPLE__
        fputs( "; Use DoubleBuffering\ndoublebuf=1\n\n", fd );
        fputs( "; Use OpenGL\ngl=1\n\n", fd );
#else
        fputs( "; Use DoubleBuffering\ndoublebuf=0\n\n", fd );
        fputs( "; Use OpenGL\ngl=0\n\n", fd );
        fputs( "; Location of the datafiles\ndatadir=", fd );
        fputs( ASSETDIR "\n\n", fd );
#endif
        fputs( "; Use mono audio only\nmono=0\n\n", fd );
        fputs( "; Grab the mouse to the window\ngrabmouse=0\n\n", fd );
        fputs( "; Set the scale factor\nscale=2\n\n", fd );
        fputs( "; Use anti-aliasing (with gl=1 only)\nantialias=1\n\n", fd );
//        fputs( "; Set the width of the window\nx=320\n\n", fd );
//        fputs( "; Set the height of the window\ny=200\n\n", fd );
        fputs( "; Disable the SDL parachute in the case of a crash\nnosdlparachute=0\n\n", fd );
        fputs( "; Key mappings\n", fd );
        fputs( "left=LEFT\nright=RIGHT\nup=UP\ndown=DOWN\n", fd );
        fputs( "fire=SPACE\nweapprev=CTRL_R\nweapnext=INSERT\n", fd );
        fclose( fd );
    }
    else
    {
        printf( "Unable to create 'abuserc' file.\n" );
    }
#endif
}

//
// Read in the 'abuserc' file
//
void readRCFile()
{
#if (defined(__wii__) || defined(__gamecube__))
    // not useful for Wii
    return;
#else
    FILE *fd = NULL;
    char *rcfile;
    char buf[255];
    char *result;

    rcfile = (char *)malloc( strlen( get_save_filename_prefix() ) + 9 );
    sprintf( rcfile, "%s/abuserc", get_save_filename_prefix() );
    if( (fd = fopen( rcfile, "r" )) != NULL )
    {
        while( fgets( buf, sizeof( buf ), fd ) != NULL )
        {
            result = strtok( buf, "=" );
            if( strcasecmp( result, "fullscreen" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.fullscreen = atoi( result );
            }
            else if( strcasecmp( result, "doublebuf" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.doublebuf = atoi( result );
            }
            else if( strcasecmp( result, "mono" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.mono = atoi( result );
            }
            else if( strcasecmp( result, "grabmouse" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.grabmouse = atoi( result );
            }
            else if( strcasecmp( result, "scale" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                scale = atoi( result );
//                flags.xres = xres * atoi( result );
//                flags.yres = yres * atoi( result );
            }
/*            else if( strcasecmp( result, "x" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.xres = atoi( result );
            }
            else if( strcasecmp( result, "y" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.yres = atoi( result );
            }*/
            else if( strcasecmp( result, "gl" ) == 0 )
            {
                // We leave this in even if we don't have OpenGL so we can
                // at least inform the user.
                result = strtok( NULL, "\n" );
                flags.gl = atoi( result );
            }
#ifdef HAVE_OPENGL
            else if( strcasecmp( result, "antialias" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                if( atoi( result ) )
                {
                    flags.antialias = GL_LINEAR;
                }
            }
#endif
            else if( strcasecmp( result, "nosdlparachute" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.nosdlparachute = atoi( result );
            }
            else if( strcasecmp( result, "datadir" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                set_filename_prefix( result );
            }
            else if( strcasecmp( result, "left" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.left = key_value( result );
            }
            else if( strcasecmp( result, "right" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.right = key_value( result );
            }
            else if( strcasecmp( result, "up" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.up = key_value( result );
            }
            else if( strcasecmp( result, "down" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.down = key_value( result );
            }
            else if( strcasecmp( result, "fire" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b2 = key_value( result );
            }
            else if( strcasecmp( result, "special" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b1 = key_value( result );
            }
            else if( strcasecmp( result, "weapprev" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b3 = key_value( result );
            }
            else if( strcasecmp( result, "weapnext" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b4 = key_value( result );
            }
        }
        fclose( fd );
    }
    else
    {
        // Couldn't open the abuserc file so let's create a default one
        createRCFile( rcfile );
    }
    free( rcfile );
#endif
}

//
// Parse the command-line parameters
//
void parseCommandLine( int argc, char **argv )
{
    for( int ii = 1; ii < argc; ii++ )
    {
        if( !strcasecmp( argv[ii], "-fullscreen" ) )
        {
            flags.fullscreen = 1;
        }
        else if( !strcasecmp( argv[ii], "-doublebuf" ) )
        {
            flags.doublebuf = 1;
        }
        else if( !strcasecmp( argv[ii], "-size" ) )
        {
            if( ii + 1 < argc && !sscanf( argv[++ii], "%d", &xres ) )
            {
                xres = 320;
            }
            if( ii + 1 < argc && !sscanf( argv[++ii], "%d", &yres ) )
            {
                yres = 200;
            }
        }
        else if( !strcasecmp( argv[ii], "-scale" ) )
        {
            int result;
            if( sscanf( argv[++ii], "%d", &result ) )
            {
                scale = result;
/*                flags.xres = xres * scale;
                flags.yres = yres * scale; */
            }
        }
/*        else if( !strcasecmp( argv[ii], "-x" ) )
        {
            int x;
            if( sscanf( argv[++ii], "%d", &x ) )
            {
                flags.xres = x;
            }
        }
        else if( !strcasecmp( argv[ii], "-y" ) )
        {
            int y;
            if( sscanf( argv[++ii], "%d", &y ) )
            {
                flags.yres = y;
            }
        }*/
        else if( !strcasecmp( argv[ii], "-nosound" ) )
        {
            flags.nosound = 1;
        }
        else if( !strcasecmp( argv[ii], "-gl" ) )
        {
            // We leave this in even if we don't have OpenGL so we can
            // at least inform the user.
            flags.gl = 1;
        }
#ifdef HAVE_OPENGL
        else if( !strcasecmp( argv[ii], "-antialias" ) )
        {
            flags.antialias = GL_LINEAR;
        }
#endif
        else if( !strcasecmp( argv[ii], "-mono" ) )
        {
            flags.mono = 1;
        }
        else if( !strcasecmp( argv[ii], "-datadir" ) )
        {
            char datadir[255];
            if( ii + 1 < argc && sscanf( argv[++ii], "%s", datadir ) )
            {
                set_filename_prefix( datadir );
            }
        }
        else if( !strcasecmp( argv[ii], "-h" ) || !strcasecmp( argv[ii], "--help" ) )
        {
            showHelp();
            exit( 0 );
        }
#if (defined(__wii__) || defined(__gamecube__))
        else if( !strcasecmp( argv[ii], "-widestretch" ) )
        {
            flags.widestretch = 1;
        }
        else if( !strcasecmp( argv[ii], "-swapbuttons" ) )
        {
            flags.swapbuttons = 1;
        }
        else if( !strcasecmp( argv[ii], "-usevaxis" ) )
        {
            flags.usevaxis = 1;
        }
        else if( !strcasecmp( argv[ii], "-hdeadzone" ) )
        {
            float h;
            if( sscanf( argv[++ii], "%f", &h ) &&
                h > 0.0 && h < 100.0)
            {
                flags.hdeadzone = static_cast<int>(32768.0 * 0.01 * h);
            }
//            printf("hdeadzone: argv[%d]=%s -> %f -> %d\n", ii, argv[ii], h, flags.hdeadzone);
        }
        else if( !strcasecmp( argv[ii], "-vdeadzone" ) )
        {
            float v;
            if( sscanf( argv[++ii], "%f", &v ) &&
                v > 0.0 && v < 100.0)
            {
                flags.vdeadzone = static_cast<int>(32768.0 * 0.01 * v);
            }
//            printf("vdeadzone: argv[%d]=%s -> %f -> %d\n", ii, argv[ii], v, flags.vdeadzone);
        }
#endif
    }
}

//
// Setup SDL and configuration
//
void setup( int argc, char **argv )
{
    // Initialise default settings
    flags.fullscreen        = 0;            // Start in a window
    flags.mono              = 0;            // Enable stereo sound
    flags.nosound           = 0;            // Enable sound
    flags.grabmouse         = 0;            // Don't grab the mouse
    flags.nosdlparachute    = 0;            // SDL error handling
    flags.xres = xres       = 320;          // Default window width
    flags.yres = yres       = 200;          // Default window height
    scale                   = 2;            // Default scale amount
#ifdef __APPLE__
    flags.gl                = 1;            // Use opengl
    flags.doublebuf         = 1;            // Do double buffering
#elif (defined(__wii__) || defined(__gamecube__))
    flags.gl                = 0;            // Don't use opengl
    flags.doublebuf         = 1;            // Do double buffering
    flags.widestretch       = 0;            // Don't stretch in widescreen
    flags.swapbuttons       = 0;            // Don't swap buttons
    flags.usevaxis          = 0;            // Don't use vertical axis
    flags.hdeadzone         = static_cast<int>(32768.0 * 0.15); // Default horizontal deadzone
    flags.vdeadzone         = static_cast<int>(32768.0 * 0.30); // Default vertical deadzone
    scale                   = 1;            // Default Wii to a scale of 1, which causes a bit of smoothing
#else
    flags.gl                = 0;            // Don't use opengl
    flags.doublebuf         = 0;            // No double buffering
#endif
#ifdef HAVE_OPENGL
    flags.antialias         = GL_NEAREST;   // Don't anti-alias
#endif
    keys.up                 = key_value( "UP" );
    keys.down               = key_value( "DOWN" );
    keys.left               = key_value( "LEFT" );
    keys.right              = key_value( "RIGHT" );
    keys.b3                 = key_value( "CTRL_R" );
    keys.b4                 = key_value( "INSERT" );

    // Display our name and version
    printf( "%s %s\n", PACKAGE_NAME, PACKAGE_VERSION );

#if (defined(__wii__) || defined(__gamecube__))
    // Initialize Wii filesystem so that disk I/O can occur
    if(!fatInitDefault())
    {
        printf( "WARNING: Unable to initialize filesystem(s).\n" );
        printf( "         This may result in an inability to read/write game files.\n" );
    }

    // Initialize Wii SDL with video, audio and joystick support
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 )
#else
    // Initialize SDL with video and audio support
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
#endif
    {
        printf( "Unable to initialise SDL : %s\n", SDL_GetError() );
        exit( 1 );
    }
    atexit( SDL_Quit );

#if (defined(__wii__) || defined(__gamecube__))
    // Init joystick and enable SDL joystick event generation mode
    SDL_JoystickEventState(SDL_ENABLE);

    if (!SDL_JoystickOpen(0))
    {
        printf( "WARNING: SDL_JoystickOpen(0) returned NULL.\n" );
        printf( "         Ensure that Wiimote is powered on and connected.\n" );
    }
#endif

    // Set the savegame directory
    char *homedir;
    char *savedir;

#if (defined(__wii__) || defined(__gamecube__))
    // On Wii, savedir is stored as a save/ subdirectory of the path
    // containing the game binary. Wii needs absolute paths for some reason, so
    // determine the path the binary is running from and use that as a base.
    savedir = NULL;
    const char* savedirname = "save/";
    int savedirlen(strlen(savedirname));

    // find slash at end of last directory name
    for (int i = strlen(argv[0]) - 1; i >= 0; --i)
    {
        if (argv[0][i] == '/')
        {
            savedir = new char[i + savedirlen + 2];
            if (savedir != NULL)
            {
                savedir[i + savedirlen + 1] = '\0';
                strncpy(savedir, argv[0], i + 1);
                strcpy(savedir + i + 1, savedirname);
            }

            break;
        }
    }

    if (savedir == NULL)
    {
        // something went wrong, default to hard-coded path
        // this allows running via wiiload if the data/save dirs are on SD
        printf("WARNING: Unable to determine save directory path.\n");
        printf("         Will try sd:/apps/abuse/save/\n");

        set_save_filename_prefix("sd:/apps/abuse/save/");
    }
    else
    {
        set_save_filename_prefix(savedir);
        delete savedir;
    }
#else
    FILE *fd = NULL;

    if( (homedir = getenv( "HOME" )) != NULL )
    {
        savedir = (char *)malloc( strlen( homedir ) + 9 );
        sprintf( savedir, "%s/.abuse/", homedir );
        // Check if we already have a savegame directory
        if( (fd = fopen( savedir, "r" )) == NULL )
        {
            // FIXME: Add some error checking here
            mkdir( savedir, S_IRUSR | S_IWUSR | S_IXUSR );
        }
        else
        {
            fclose( fd );
        }
        set_save_filename_prefix( savedir );
        free( savedir );
    }
    else
    {
        // Warn the user that we couldn't set the savename prefix
        printf( "WARNING: Unable to get $HOME environment variable.\n" );
        printf( "         Savegames will probably fail.\n" );
        // Just use the working directory.
        // Hopefully they have write permissions....
        set_save_filename_prefix( "" );
    }
#endif

    // Set the datadir to a default value
    // (The current directory)
#ifdef __APPLE__
    UInt8 buffer[255];
    CFURLRef bundleurl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef url = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, bundleurl, CFSTR("Contents/Resources/data"), true);

    if (!CFURLGetFileSystemRepresentation(url, true, buffer, 255))
    {
        exit(1);
    }
    else
        set_filename_prefix( (const char*)buffer );
#elif (defined(__wii__) || defined(__gamecube__))
    // On Wii, assetdir/datadir is stored as a data/ subdirectory of the path
    // containing the game binary. Wii needs absolute paths for some reason, so
    // determine the path the binary is running from and use that as a base.
    //
    // note that we're hijacking the homedir variable here because it isn't
    // otherwise used for Wii, and using it also eliminates a compiler warning.
    homedir = NULL;
    const char* assetdirname("data/");
    int assetdirlen(strlen(assetdirname));

    // find slash at end of last directory name
    for (int i = strlen(argv[0]) - 1; i >= 0; --i)
    {
        if (argv[0][i] == '/')
        {
            homedir = new char[i + assetdirlen + 2];
            if (homedir != NULL)
            {
                homedir[i + assetdirlen + 1] = '\0';
                strncpy(homedir, argv[0], i + 1);
                strcpy(homedir + i + 1, assetdirname);
            }

            break;
        }
    }

    if (homedir == NULL)
    {
        // something went wrong, default to hard-coded path
        // this allows running via wiiload if the data/save dirs are on SD
        printf("WARNING: Unable to determine data directory path.\n");
        printf("         Will try sd:/apps/abuse/data/\n");

        set_filename_prefix("sd:/apps/abuse/data/");
    }
    else
    {
//        printf("Using data path: %s\n", homedir);

        set_filename_prefix(homedir);
        delete homedir;
    }
#else
    set_filename_prefix( ASSETDIR );
#endif

    // Load the users configuration
    readRCFile();

    // Handle command-line parameters
    parseCommandLine( argc, argv );

    // Calculate the scaled window size.
    flags.xres = xres * scale;
    flags.yres = yres * scale;

    // Stop SDL handling some errors
    if( flags.nosdlparachute )
    {
        // segmentation faults
        signal( SIGSEGV, SIG_DFL );
        // floating point errors
        signal( SIGFPE, SIG_DFL );
    }
}

//
// Get the key binding for the requested function
//
int get_key_binding(char const *dir, int i)
{
    if( strcasecmp( dir, "left" ) == 0 )
        return keys.left;
    else if( strcasecmp( dir, "right" ) == 0 )
        return keys.right;
    else if( strcasecmp( dir, "up" ) == 0 )
        return keys.up;
    else if( strcasecmp( dir, "down" ) == 0 )
        return keys.down;
    else if( strcasecmp( dir, "b1" ) == 0 )
        return keys.b1;
    else if( strcasecmp( dir, "b2" ) == 0 )
        return keys.b2;
    else if( strcasecmp( dir, "b3" ) == 0 )
        return keys.b3;
    else if( strcasecmp( dir, "b4" ) == 0 )
        return keys.b4;

    return 0;
}
