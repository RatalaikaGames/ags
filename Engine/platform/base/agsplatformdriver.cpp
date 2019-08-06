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

#include <thread>
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
#include "ac/timer.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;
using namespace AGS::Engine;

#if defined (AGS_HAS_CD_AUDIO)
#include "libcda.h"
#endif

// We don't have many places where we delay longer than a frame, but where we
// do, we should give the audio layer a chance to update.
// 16 milliseconds is rough period for 60fps
const auto MaximumDelayBetweenPolling = std::chrono::milliseconds(16);

AGSPlatformDriver* AGSPlatformDriver::instance = nullptr;
AGSPlatformDriver *platform = nullptr;

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
void AGSPlatformDriver::PlayVideo(const char* name, int skip, int flags) {}

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
    time_t t = time(nullptr);

    //note: subject to year 2038 problem due to shoving time_t in an integer
    sdt->rawUnixTime = time( &long_time );
    newtime = localtime( &long_time );

    struct tm *newtime = localtime(&t);
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
    fflush(stdout);
}

void AGSPlatformDriver::YieldCPU() {
    // NOTE: this is called yield, but if we actually yield instead of delay,
    // we get a massive increase in CPU usage.
    this->Delay(1);
    //std::this_thread::yield();
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

Common::Stream *AGSPlatformDriver::OpenFile(eFilePurpose purpose, const Common::String &filename, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    return File::OpenFile(filename, open_mode, work_mode);
}

Stream* AGSPlatformDriver::Save_CreateSlotStream(int slnum)
{
    String nametouse;
    nametouse = get_save_game_path(slnum);

    return Common::File::CreateFile(nametouse);
}

struct PfStruct
{
    FILE* fp;
    long length;
    int64_t offset;
};

void* AGSPlatformDriver::allegro_fopen(eFilePurpose purpose, const char* path, int64_t offset, long length)
{
    FILE* fp = fopen(path,"rb");
    if(!fp) return NULL;
    PfStruct* ret = new PfStruct();
    ret->fp = fp;
    ret->length = length;
    ret->offset = offset;
    return ret;
}
int AGSPlatformDriver::allegro_fclose(void *userdata)
{
    PfStruct* pf = (PfStruct*)userdata;
    int ret = fclose(pf->fp);
    delete pf;
    return ret;
}
int AGSPlatformDriver::allegro_getc(void *userdata)
{
    return fgetc(((PfStruct*)userdata)->fp);
}
int AGSPlatformDriver::allegro_ungetc(int c, void *userdata)
{
    return ungetc(c, ((PfStruct*)userdata)->fp);
}
long AGSPlatformDriver::allegro_fread(void *p, long n, void *userdata)
{
    PfStruct* pf = ((PfStruct*)userdata);
    long remain = allegro_fremain(userdata);
    if(n>remain) n = remain;
    return fread(p, 1, n, ((PfStruct*)userdata)->fp);
}
int AGSPlatformDriver::allegro_putc(int c, void *userdata)
{
    return fputc(c, ((PfStruct*)userdata)->fp);
}
long AGSPlatformDriver::allegro_fwrite(const void *p, long n, void *userdata)
{
    return fwrite(p, 1, n, ((PfStruct*)userdata)->fp);
}
int AGSPlatformDriver::allegro_fseek(void *userdata, int offset) { 
    PfStruct* pf = ((PfStruct*)userdata);
    return fseek(pf->fp, offset, SEEK_CUR);
}
int AGSPlatformDriver::allegro_feof(void *userdata) { 
    PfStruct* pf = ((PfStruct*)userdata);
    return feof(pf->fp);
}
int AGSPlatformDriver::allegro_ferror(void *userdata)
{
    return ferror(((PfStruct*)userdata)->fp);
}
long AGSPlatformDriver::allegro_flength(void *userdata)
{
    return ((PfStruct*)userdata)->length;
}
long AGSPlatformDriver::allegro_fremain(void *userdata) {
    PfStruct* pf = ((PfStruct*)userdata);
    return pf->offset + pf->length - ftell(pf->fp);
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

void AGSPlatformDriver::Delay(int millis) {
  auto now = AGS_Clock::now();
  auto delayUntil = now + std::chrono::milliseconds(millis);

  for (;;) {
    if (now >= delayUntil) { break; }

    auto duration = std::min<std::chrono::nanoseconds>(delayUntil - now, MaximumDelayBetweenPolling);
    std::this_thread::sleep_for(duration);
    now = AGS_Clock::now(); // update now

    if (now >= delayUntil) { break; }

    // don't allow it to check for debug messages, since this Delay()
    // call might be from within a debugger polling loop
    update_polled_mp3();
    now = AGS_Clock::now(); // update now
  }
}
