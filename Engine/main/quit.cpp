//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

//
// Quit game procedure
//

#include <stdio.h>

#include "ac/cdaudio.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/roomstatus.h"
#include "ac/translation.h"
#include "debug/agseditordebugger.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/main.h"
#include "main/mainheader.h"
#include "main/quit.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"
#include "plugin/plugin_engine.h"
#include "util/posix.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStruct thisroom;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro
extern int our_eip;
extern GameSetup usetup;
extern char pexbuf[STD_BUFFER_SIZE];
extern int proper_exit;
extern char check_dynamic_sprites_at_exit;
extern int editor_debugging_initialized;
extern IAGSEditorDebugger *editor_debugger;
extern int need_to_stop_cd;
extern int use_cdplayer;
extern IGraphicsDriver *gfxDriver;

bool handledErrorInEditor;

void quit_tell_editor_debugger(const String &qmsg, QuitReason qreason)
{
    if (editor_debugging_initialized)
    {
        if (qreason & kQuitKind_GameException)
            handledErrorInEditor = send_exception_to_editor(qmsg);
        send_message_to_editor("EXIT");
        editor_debugger->Shutdown();
    }
}

void quit_stop_cd()
{
    if (need_to_stop_cd)
        cd_manager(3,0);
}

void quit_shutdown_scripts()
{
    ccUnregisterAllObjects();
}

void quit_check_dynamic_sprites(QuitReason qreason)
{
    if ((qreason & kQuitKind_NormalExit) && (check_dynamic_sprites_at_exit) && 
        (game.options[OPT_DEBUGMODE] != 0)) {
            // game exiting normally -- make sure the dynamic sprites
            // have been deleted
            for (int i = 1; i < spriteset.GetSpriteSlotCount(); i++) {
                if (game.SpriteInfos[i].Flags & SPF_DYNAMICALLOC)
                    debug_script_warn("Dynamic sprite %d was never deleted", i);
            }
    }
}

void quit_shutdown_platform(QuitReason qreason)
{
    // Be sure to unlock mouse on exit, or users will hate us
    platform->UnlockMouse();
    platform->AboutToQuitGame();

    our_eip = 9016;

    pl_stop_plugins();

    quit_check_dynamic_sprites(qreason);

    platform->FinishedUsingGraphicsMode();

    if (use_cdplayer)
        platform->ShutdownCDPlayer();
}

void quit_shutdown_audio()
{
    our_eip = 9917;
    game.options[OPT_CROSSFADEMUSIC] = 0;
    stopmusic();
#ifndef PSP_NO_MOD_PLAYBACK
    if (opts.mod_player)
        remove_mod_player();
#endif

    // Quit the sound thread.
    audioThread.Stop();

    remove_sound();
}

QuitReason quit_check_for_error_state(const char *&qmsg, String &alertis)
{
    if (qmsg[0]=='|')
    {
        return kQuit_GameRequest;
    }
    else if (qmsg[0]=='!')
    {
        QuitReason qreason;
        qmsg++;

        if (qmsg[0] == '|')
        {
            qreason = kQuit_UserAbort;
            alertis = "Abort key pressed.\n\n";
        }
        else if (qmsg[0] == '?')
        {
            qmsg++;
            qreason = kQuit_ScriptAbort;
            alertis = "A fatal error has been generated by the script using the AbortGame function. Please contact the game author for support.\n\n";
        }
        else
        {
            qreason = kQuit_GameError;
            alertis.Format("An error has occurred. Please contact the game author for support, as this "
                "is likely to be a scripting error and not a bug in AGS.\n"
                "(ACI version %s)\n\n", EngineVersion.LongString.GetCStr());
        }

        alertis.Append(get_cur_script(5));

        if (qreason != kQuit_UserAbort)
            alertis.Append("\nError: ");
        else
            qmsg = "";
        return qreason;
    }
    else if (qmsg[0] == '%')
    {
        qmsg++;
        alertis.Format("A warning has been generated. This is not normally fatal, but you have selected "
            "to treat warnings as errors.\n"
            "(ACI version %s)\n\n%s\n", EngineVersion.LongString.GetCStr(), get_cur_script(5).GetCStr());
        return kQuit_GameWarning;
    }
    else
    {
        alertis.Format("An internal error has occurred. Please note down the following information.\n"
        "If the problem persists, post the details on the AGS Technical Forum.\n"
        "(ACI version %s)\n"
        "\nError: ", EngineVersion.LongString.GetCStr());
        return kQuit_FatalError;
    }
}

void quit_message_on_exit(const char *qmsg, String &alertis, QuitReason qreason)
{
    // successful exit displays no messages (because Windoze closes the dos-box
    // if it is empty).
    if ((qreason & kQuitKind_NormalExit) == 0 && !handledErrorInEditor)
    {
        // Display the message (at this point the window still exists)
        sprintf(pexbuf,"%s\n",qmsg);
        alertis.Append(pexbuf);
        platform->DisplayAlert(alertis);
    }
}

void quit_release_data()
{
    resetRoomStatuses();
    thisroom.Free();
    play.Free();

    /*  _CrtMemState memstart;
    _CrtMemCheckpoint(&memstart);
    _CrtMemDumpStatistics( &memstart );*/

    Common::AssetManager::DestroyInstance();
}

void quit_delete_temp_files()
{
    #ifdef CONSOLE_VERSION
    return;
    #endif

    al_ffblk	dfb;
    int	dun = al_findfirst("~ac*.tmp",&dfb,FA_SEARCH);
    while (!dun) {
        ags_unlink(dfb.name);
        dun = al_findnext(&dfb);
    }
    al_findclose (&dfb);
}

void free_globals()
{
#if defined (WINDOWS_VERSION)
    if (wArgv)
    {
        LocalFree(wArgv);
        wArgv = NULL;
    }
#endif
}

// TODO: move to test unit
extern Bitmap *test_allegro_bitmap;
extern IDriverDependantBitmap *test_allegro_ddb;
void allegro_bitmap_test_release()
{
	delete test_allegro_bitmap;
	if (test_allegro_ddb)
		gfxDriver->DestroyDDB(test_allegro_ddb);
}

char return_to_roomedit[30] = "\0";
char return_to_room[150] = "\0";
// quit - exits the engine, shutting down everything gracefully
// The parameter is the message to print. If this message begins with
// an '!' character, then it is printed as a "contact game author" error.
// If it begins with a '|' then it is treated as a "thanks for playing" type
// message. If it begins with anything else, it is treated as an internal
// error.
// "!|" is a special code used to mean that the player has aborted (Alt+X)
void quit(const char *quitmsg)
{
    String alertis;
    QuitReason qreason = quit_check_for_error_state(quitmsg, alertis);
    // Need to copy it in case it's from a plugin (since we're
    // about to free plugins)
    String qmsg = quitmsg;

    #ifndef CONSOLE_VERSION
    if (qreason & kQuitKind_NormalExit)
        save_config_file();
    #endif

	allegro_bitmap_test_release();

    handledErrorInEditor = false;

    quit_tell_editor_debugger(qmsg, qreason);

    our_eip = 9900;

    quit_stop_cd();

    our_eip = 9020;

    quit_shutdown_scripts();

    quit_shutdown_platform(qreason);

    our_eip = 9019;

    quit_shutdown_audio();
    
    our_eip = 9901;

    shutdown_font_renderer();
    our_eip = 9902;

    spriteset.Reset();

    our_eip = 9907;

    close_translation();

    our_eip = 9908;

    engine_shutdown_gfxmode();

    quit_message_on_exit(qmsg, alertis, qreason);

    quit_release_data();

    // release backed library
    // WARNING: no Allegro objects should remain in memory after this,
    // if their destruction is called later, program will crash!
    allegro_exit();

    platform->PostAllegroExit();

    our_eip = 9903;

    quit_delete_temp_files();

    proper_exit=1;

    Debug::Printf(kDbgMsg_Init, "***** ENGINE HAS SHUTDOWN");

    shutdown_debug();
    free_globals();

    our_eip = 9904;
    exit(EXIT_NORMAL);
}

extern "C" {
    void quit_c(char*msg) {
        quit(msg);
    }
}
