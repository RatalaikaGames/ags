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
#include "util/string_utils.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "plugin/agsplugin.h"
#include "ac/timer.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// We don't have many places where we delay longer than a frame, but where we
// do, we should give the audio layer a chance to update.
// 16 milliseconds is rough period for 60fps
const auto MaximumDelayBetweenPolling = std::chrono::milliseconds(16);

AGSPlatformDriver* AGSPlatformDriver::instance = nullptr;
bool AGSPlatformDriver::_logToStdErr = false;
bool AGSPlatformDriver::_guiMode = false;
AGSPlatformDriver *platform = nullptr;

// ******** DEFAULT IMPLEMENTATIONS *******

void AGSPlatformDriver::AboutToQuitGame() { }
void AGSPlatformDriver::PostAllegroInit(bool windowed) { }
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
    sdt->rawUnixTime = static_cast<int>(t);

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

void AGSPlatformDriver::WriteStdErr(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
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

//-----------------------------------------------
// IOutputHandler implementation
//-----------------------------------------------
void AGSPlatformDriver::PrintMessage(const Common::DebugMessage &msg)
{
    if (_logToStdErr)
    {
        if (msg.GroupName.IsEmpty())
            WriteStdErr("%s", msg.Text.GetCStr());
        else
            WriteStdErr("%s : %s", msg.GroupName.GetCStr(), msg.Text.GetCStr());
    }
    else
    {
        if (msg.GroupName.IsEmpty())
            WriteStdOut("%s", msg.Text.GetCStr());
        else
            WriteStdOut("%s : %s", msg.GroupName.GetCStr(), msg.Text.GetCStr());
    }
}


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
