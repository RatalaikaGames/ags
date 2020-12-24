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
// AGS Cross-Platform Header
//
//=============================================================================

#ifndef __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
#define __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H

#include <errno.h>
#include <vector>
#include "ac/datetime.h"
#include "debug/outputhandler.h"
#include "util/ini_util.h"
#include "util/stream.h"
#include "util/file.h"

namespace AGS
{
    namespace Common { class Stream; }
    namespace Engine { struct DisplayMode; }
}
using namespace AGS; // FIXME later

enum eScriptSystemOSID
{
    eOS_DOS = 1,
    eOS_Win,
    eOS_Linux,
    eOS_Mac,
    eOS_Android,
    eOS_iOS,
    eOS_PSP,
    eOS_PSVita,
    eOS_PSX,
    eOS_PS2,
    eOS_PS3,
    eOS_PS4,
    eOS_PS5,
    eOS_Xbox,
    eOS_Xbox360,
    eOS_XboxOne,
    eOS_XboxSeriesX,
    eOS_N64,
    eOS_Wii,
    eOS_WiiU,
    eOS_GBA,
    eOS_NDS,
    eOS_N3DS,
    eOS_NSwitch,
    
    //Oftentimes, people will have a windows prototype while developing for consoles.
    //This is here to distinguish it from the eOS_Win case, which would be the legit windows release
    eOS_WinProto 
};

enum eFilePurpose
{
    eFilePurpose_LoadAsset,
    eFilePurpose_MountPackfile
};

enum SetupReturnValue
{
    kSetup_Cancel,
    kSetup_Done,
    kSetup_RunGame
};

struct AGSPlatformDriver
    // be used as a output target for logging system
    : public AGS::Common::IOutputHandler
{
    virtual void AboutToQuitGame();
    virtual void Delay(int millis);
    virtual void DisplayAlert(const char*, ...) = 0;
    virtual int  GetLastSystemError() { return errno; }
    // Get root directory for storing per-game shared data
    virtual const char *GetAllUsersDataDirectory() { return "."; }
    // Get root directory for storing per-game saved games
    virtual const char *GetUserSavedgamesDirectory() { return "."; }
    // Get root directory for storing per-game user configuration files
    virtual const char *GetUserConfigDirectory() { return "."; }
    // Get directory for storing all-games user configuration files
    virtual const char *GetUserGlobalConfigDirectory()  { return "."; }
    // Get default directory for program output (logs)
    virtual const char *GetAppOutputDirectory() { return "."; }
    // Returns array of characters illegal to use in file names
    virtual const char *GetIllegalFileChars() { return "\\/"; }
    virtual const char *GetDiskWriteAccessTroubleshootingText();
    virtual const char *GetGraphicsTroubleshootingText() { return ""; }
    virtual unsigned long GetDiskFreeSpaceMB() = 0;
    // Tells whether build is capable of controlling mouse movement properly
    virtual bool IsMouseControlSupported(bool windowed) { return false; }
    // Tells whether this platform's backend library deals with mouse cursor
    // virtual->real coordinate transformation itself (otherwise AGS engine should do it)
    virtual bool IsBackendResponsibleForMouseScaling() { return false; }
    virtual const char* GetAllegroFailUserHint();
    virtual eScriptSystemOSID GetSystemOSID() = 0;
    virtual void GetSystemTime(ScriptDateTime*);
    virtual void PlayVideo(const char* name, int skip, int flags);
    virtual void InitialiseAbufAtStartup();
    virtual void PostAllegroInit(bool windowed);
    virtual void PostAllegroExit();
    virtual void FinishedUsingGraphicsMode();
    virtual SetupReturnValue RunSetup(const Common::ConfigTree &cfg_in, Common::ConfigTree &cfg_out);
    virtual void SetGameWindowIcon();
    // Formats message and writes to standard platform's output;
    // Always adds trailing '\n' after formatted string
    virtual void WriteStdOut(const char *fmt, ...);
    // Formats message and writes to platform's error output;
    // Always adds trailing '\n' after formatted string
    virtual void WriteStdErr(const char *fmt, ...);
    virtual void YieldCPU();
    // Called when the game window is being switch out from
    virtual void DisplaySwitchOut();
    // Called when the game window is being switch back to
    virtual void DisplaySwitchIn();
    // Called when the application is being paused completely (e.g. when player alt+tabbed from it).
    // This function should suspend any platform-specific realtime processing.
    virtual void PauseApplication();
    // Called when the application is being resumed.
    virtual void ResumeApplication();
    // Returns a list of supported display modes
    virtual void GetSystemDisplayModes(std::vector<Engine::DisplayMode> &dms);
    // Switch to system fullscreen mode; store previous mode parameters
    virtual bool EnterFullscreenMode(const Engine::DisplayMode &dm);
    // Return back to the mode was before switching to fullscreen
    virtual bool ExitFullscreenMode();
    // Adjust application window's parameters to suit fullscreen mode
    virtual void AdjustWindowStyleForFullscreen();
    // Restore application window to normal parameters
    virtual void RestoreWindowStyle();
    virtual void RegisterGameWithGameExplorer();
    virtual void UnRegisterGameWithGameExplorer();
    virtual int  ConvertKeycodeToScanCode(int keyCode);
    // Adjust window size to ensure it is in the supported limits
    virtual void ValidateWindowSize(int &x, int &y, bool borderless) const {}

    virtual int  InitializeCDPlayer();  // return 0 on success
    virtual int  CDPlayerCommand(int cmdd, int datt);
    virtual void ShutdownCDPlayer();

    virtual bool LockMouseToWindow();
    virtual void UnlockMouse();

    //called whenever the game script SetTranslation is called (so fonts can be changed, etc.)
    virtual void SetTranslation(const char* name);

    //Waits for next frame, by monitoring timerloop which is updated by an asynchronous process (or some other platform-specific mechanism)
    virtual void WaitForNextFrame();

    virtual Common::Stream* Save_CreateSlotStream(int slnum);
    virtual void Save_DeleteSlot(int slnum);

    virtual Common::Stream *OpenFile(eFilePurpose purpose, const Common::String &filename, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);

    //-----------------------
    //New functions which only had posix implementations but need to be done differently on non-posix systems

    //(these are named with Directory in front to avoid conflicts with win32 macros that are difficult to avoid)
    // Creates new directory (if it does not exist)
    // Returns true if the directory now exists (whether or not it previously existed)
    virtual bool DirectoryCreate(const Common::String &path);
    
    // Sets current working directory, returns the resulting path
    virtual Common::String DirectorySetCurrent(const Common::String &path);
    
    // Makes sure all directories in the path are created. Parent path is
    // not touched, and function must fail if parent path is not accessible.
    virtual bool DirectoryCreateAll(const Common::String &parent, const Common::String &path);

    // Gets current working directory
    virtual Common::String DirectoryGetCurrent();

    // Indicates whether the directory exists (path exists, and is a directory)
    virtual bool DirectoryExists(const Common::String &path);

    //deletes a file path
    virtual bool PathDelete(const Common::String &path);

    //renames a path
    virtual bool PathRename(const Common::String &oldpath, const Common::String &newpath);

    // Functions for allegro plumbing so it can run through virtualized file IO (only used by packfile system)
    // Due to this, I've only bothered to set it up for opening files for reading
    // TODO - these have been specialized to be odd packfile semantics; the functions should be renamed accordingly
    virtual void* allegro_fopen(eFilePurpose purpose, const char* path, int64_t offset, long length); //long and not 64bit because the ftell API returns longs anyway
    virtual int allegro_fclose(void *userdata);
    virtual int allegro_getc(void *userdata);
    virtual int allegro_ungetc(int c, void *userdata);
    virtual long allegro_fread(void *p, long n, void *userdata);
    virtual int allegro_putc(int c, void *userdata);
    virtual long allegro_fwrite(const void *p, long n, void *userdata);
    virtual int allegro_fseek(void *userdata, int offset); //warning! this is implicitly SEEK_CUR
    virtual int allegro_feof(void *userdata);
    virtual int allegro_ferror(void *userdata);
    
    //these functions are extensions, because allegro lacks them. it's picky details of how allegro and AGS work together to use the packfile
    virtual long allegro_flength(void *userdata); 
    virtual long allegro_fremain(void *userdata); 

    static AGSPlatformDriver *GetDriver();
    // Set whether PrintMessage should output to stdout or stderr
    static void SetOutputToErr(bool on) { _logToStdErr = on; }
    // Set whether DisplayAlert is allowed to show modal GUIs on some systems;
    // it will print to either stdout or stderr otherwise, depending on above flag
    static void SetGUIMode(bool on) { _guiMode = on; }

    //-----------------------------------------------
    // IOutputHandler implementation
    //-----------------------------------------------
    // Writes to the standard platform's output, prepending "AGS: " prefix to the message
    void PrintMessage(const AGS::Common::DebugMessage &msg) override;

protected:
    // TODO: this is a quick solution for IOutputHandler implementation
    // logging either to stdout or stderr. Normally there should be
    // separate implementation, one for each kind of output, but
    // with both going through PlatformDriver need to figure a better
    // design first.
    static bool _logToStdErr;
    // Defines whether engine is allowed to display important warnings
    // and errors by showing a message box kind of GUI.
    static bool _guiMode;

    //-----------------------------------------------
    // UNUSED THINGS. SHOULD BE DELETED?
    //-----------------------------------------------
    virtual const char* _deprecated_GetNoMouseErrorString() { return NULL; }
    //-----------------------------------------------


private:
    static AGSPlatformDriver *instance;
};

#if defined (AGS_HAS_CD_AUDIO)
int cd_player_init();
int cd_player_control(int cmdd, int datt);
#endif

// [IKM] What is a need to have this global var if you can get AGSPlatformDriver
// instance by calling AGSPlatformDriver::GetDriver()?
extern AGSPlatformDriver *platform;

#endif // __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
