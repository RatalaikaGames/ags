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
// AGS Platform-specific functions
//
//=============================================================================

#include <stdio.h>

#include "util/wgt2allg.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/common.h"
#include "ac/runtime_defines.h"
#include "ac/game.h"
#include "util/string_utils.h"
#include "util/stream.h"
#include "util/posix.h"
#include "util/file.h"
#include "game/main_game_file.h"
#include "game/savegame.h"
#include "gfx/bitmap.h"
#include "plugin/agsplugin.h"

using namespace AGS::Common;
using namespace AGS::Engine;

#if defined (AGS_HAS_CD_AUDIO)
#include "libcda.h"
#endif

AGSPlatformDriver* AGSPlatformDriver::instance = NULL;
AGSPlatformDriver *platform = NULL;

// ******** DEFAULT IMPLEMENTATIONS *******

void AGSPlatformDriver::AboutToQuitGame() { }
void AGSPlatformDriver::PostAllegroInit(bool windowed) { }
void AGSPlatformDriver::PostAllegroExit() {}
void AGSPlatformDriver::DisplaySwitchOut() { }
void AGSPlatformDriver::DisplaySwitchIn() { }
void AGSPlatformDriver::PauseApplication() { }
void AGSPlatformDriver::ResumeApplication() { }
void AGSPlatformDriver::GetSystemDisplayModes(std::vector<DisplayMode> &dms) { }
bool AGSPlatformDriver::EnterFullscreenMode(const DisplayMode &dm) { return true; }
bool AGSPlatformDriver::ExitFullscreenMode() { return true; }
void AGSPlatformDriver::AdjustWindowStyleForFullscreen() { }
void AGSPlatformDriver::RestoreWindowStyle() { }
void AGSPlatformDriver::RegisterGameWithGameExplorer() { }
void AGSPlatformDriver::UnRegisterGameWithGameExplorer() { }

int AGSPlatformDriver::InitializeCDPlayer() { return 1; }
int AGSPlatformDriver::CDPlayerCommand(int cmdd, int datt) { return 0; }
void AGSPlatformDriver::ShutdownCDPlayer() {}

void AGSPlatformDriver::PlayVideo(const char* name, int skip, int flags) {}

const char* AGSPlatformDriver::GetAllegroFailUserHint()
{
    return "Make sure you have latest version of Allegro 4 libraries installed, and your system is running in graphical mode.";
}

const char *AGSPlatformDriver::GetDiskWriteAccessTroubleshootingText()
{
    return "Make sure you have write permissions, and also check the disk's free space.";
}

void AGSPlatformDriver::GetSystemTime(ScriptDateTime *sdt) {
    struct tm *newtime;
    time_t long_time;

    //TODO CONSOLE - run time() through platform driver also.. probably. may be an example of how to deal with some of the libc problems generally
    time( &long_time );
    newtime = localtime( &long_time );

    sdt->hour = newtime->tm_hour;
    sdt->minute = newtime->tm_min;
    sdt->second = newtime->tm_sec;
    sdt->day = newtime->tm_mday;
    sdt->month = newtime->tm_mon + 1;
    sdt->year = newtime->tm_year + 1900;
}

void AGSPlatformDriver::WriteStdOut(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void AGSPlatformDriver::YieldCPU() {
    this->Delay(1);
}

void AGSPlatformDriver::InitialiseAbufAtStartup()
{
    // because loading the game file accesses abuf, it must exist
    // No no no, David Blain, no magic here :P
    //abuf = BitmapHelper::CreateBitmap(10,10,8);
}

void AGSPlatformDriver::FinishedUsingGraphicsMode()
{
    // don't need to do anything on any OS except DOS
}

SetupReturnValue AGSPlatformDriver::RunSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out)
{
    return kSetup_Cancel;
}

void AGSPlatformDriver::SetGameWindowIcon() {
    // do nothing
}

int AGSPlatformDriver::ConvertKeycodeToScanCode(int keycode)
{
    keycode -= ('A' - KEY_A);
    return keycode;
}

bool AGSPlatformDriver::LockMouseToWindow() { return false; }
void AGSPlatformDriver::UnlockMouse() { }

void AGSPlatformDriver::Save_DeleteSlot(int slnum)
{
    String nametouse;
    nametouse = get_save_game_path(slnum);
    ags_unlink (nametouse);
    if ((slnum >= 1) && (slnum <= MAXSAVEGAMES)) {
        String thisname;
        for (int i = MAXSAVEGAMES; i > slnum; i--) {
            thisname = get_save_game_path(i);
            if (Common::File::TestReadFile(thisname)) {
                // Rename the highest save game to fill in the gap
                rename (thisname, nametouse);
                break;
            }
        }
    }
}

Stream* AGSPlatformDriver::Save_CreateSlotStream(int slnum)
{
    String nametouse;
    nametouse = get_save_game_path(slnum);

    return Common::File::CreateFile(nametouse);
}


//-----------------------------------------------
// IOutputHandler implementation
//-----------------------------------------------
void AGSPlatformDriver::PrintMessage(const Common::DebugMessage &msg)
{
    if (msg.GroupName.IsEmpty())
        WriteStdOut("%s", msg.Text.GetCStr());
    else
        WriteStdOut("%s : %s", msg.GroupName.GetCStr(), msg.Text.GetCStr());
}

// ********** CD Player Functions common to Win and Linux ********

#if defined (AGS_HAS_CD_AUDIO)

// from ac_cdplayer
extern int use_cdplayer;
extern int need_to_stop_cd;

int numcddrives=0;

int cd_player_init() {
    int erro = cd_init();
    if (erro) return -1;
    numcddrives=1;
    use_cdplayer=1;
    return 0;
}

int cd_player_control(int cmdd, int datt) {
    // WINDOWS & LINUX VERSION
    if (cmdd==1) {
        if (cd_current_track() > 0) return 1;
        return 0;
    }
    else if (cmdd==2) {
        cd_play_from(datt);
        need_to_stop_cd=1;
    }
    else if (cmdd==3) 
        cd_pause();
    else if (cmdd==4) 
        cd_resume();
    else if (cmdd==5) {
        int first,last;
        if (cd_get_tracks(&first,&last)==0)
            return (last-first)+1;
        else return 0;
    }
    else if (cmdd==6)
        cd_eject();
    else if (cmdd==7)
        cd_close();
    else if (cmdd==8)
        return numcddrives;
    else if (cmdd==9) ;
    else quit("!CDAudio: Unknown command code");

    return 0;
}

#endif // AGS_HAS_CD_AUDIO
