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
#include <memory>
#include <vector>
#include "ac/datetime.h"
#include "ac/path_helper.h"
#include "debug/outputhandler.h"
#include "util/ini_util.h"
#include "util/stream.h"
#include "util/file.h"

struct SDL_Surface;

namespace AGS
{
    namespace Common { class Stream; }
    namespace Engine { struct DisplayMode; }
}
using namespace AGS; // FIXME later

enum eScriptSystemOSID
{
    eOS_Unknown = 0,
    eOS_DOS,
    eOS_Win,
    eOS_Linux,
    eOS_Mac,
    eOS_Android,
    eOS_iOS,
    eOS_PSP,
    eOS_Web,
    eOS_FreeBSD,
    eNumOS
#ifdef AGS_RATA
    ,
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
#endif
};

enum SetupReturnValue
{
    kSetup_Cancel,
    kSetup_Done,
    kSetup_RunGame
};

class AGSPlatformDriver
    // be used as a output target for logging system
    : public AGS::Common::IOutputHandler
{
public:
    virtual ~AGSPlatformDriver() = default;

    // Called at the creation of the platform driver
    virtual void MainInit() { };
    // Called right before the formal backend init
    virtual void PreBackendInit() { };
    // Called right after the formal backend init
    virtual void PostBackendInit() { };
    // Called right before the backend is deinitialized
    virtual void PreBackendExit() { };
    // Called right after the backend is deinitialized
    virtual void PostBackendExit() { };

    virtual void Delay(int millis);
    virtual void DisplayAlert(const char*, ...) = 0;
    virtual void AttachToParentConsole();
    virtual int  GetLastSystemError() { return errno; }
    // platform specific data file
    virtual const char * GetGameDataFile() {return nullptr; }
    // Optionally fill in config tree from the platform-specific config source
    virtual void ReadConfiguration(Common::ConfigTree &cfg) { };
    // Get root directory for storing per-game shared data
    virtual FSLocation GetAllUsersDataDirectory() { return FSLocation("."); }
    // Get root directory for storing per-game saved games
    virtual FSLocation GetUserSavedgamesDirectory() { return FSLocation("."); }
    // Get root directory for storing per-game user configuration files
    virtual FSLocation GetUserConfigDirectory() { return FSLocation("."); }
    // Get directory for storing all-games user configuration files
    virtual FSLocation GetUserGlobalConfigDirectory()  { return FSLocation("."); }
    // Get default directory for program output (logs)
    virtual FSLocation GetAppOutputDirectory() { return FSLocation("."); }
    // Returns array of characters illegal to use in file names
    virtual const char *GetIllegalFileChars() { return "\\/"; }
    virtual const char *GetDiskWriteAccessTroubleshootingText();
    virtual const char *GetGraphicsTroubleshootingText() { return ""; }
    virtual unsigned long GetDiskFreeSpaceMB() = 0;
    virtual const char* GetBackendFailUserHint();
    virtual eScriptSystemOSID GetSystemOSID() = 0;
    virtual void GetSystemTime(ScriptDateTime*);
    // Tells whether build is capable of controlling mouse movement properly
    virtual bool IsMouseControlSupported(bool windowed) { return false; }
    // Tells whether this platform's backend library deals with mouse cursor
    // virtual->real coordinate transformation itself (otherwise AGS engine should do it)
    virtual bool IsBackendResponsibleForMouseScaling() { return false; }
    virtual const char* GetAllegroFailUserHint();
    virtual SetupReturnValue RunSetup(const Common::ConfigTree &cfg_in, Common::ConfigTree &cfg_out);
    // Formats message and writes to standard platform's output;
    // Always adds trailing '\n' after formatted string
    virtual void WriteStdOut(const char *fmt, ...);
    // Formats message and writes to platform's error output;
    // Always adds trailing '\n' after formatted string
    virtual void WriteStdErr(const char *fmt, ...);
    virtual void YieldCPU();
    // Called when the application is being paused completely (e.g. when player alt+tabbed from it).
    // This function should suspend any platform-specific realtime processing.
    virtual void PauseApplication();
    // Called when the application is being resumed.
    virtual void ResumeApplication();
    // Adjust window size to ensure it is in the supported limits
    virtual void ValidateWindowSize(int &x, int &y, bool borderless) const;
    // Either set window icon using system API directly, or create a SDL_Surface
    // for the SDL backend to set an icon instead.
    virtual SDL_Surface *CreateWindowIcon() { return nullptr; }

    virtual int  InitializeCDPlayer();  // return 0 on success
    virtual int  CDPlayerCommand(int cmdd, int datt);
    virtual void ShutdownCDPlayer();

    // Returns command line argument in a UTF-8 format
    virtual Common::String GetCommandArg(size_t arg_index);

     // Allows adjusting parameters and other fixes before engine is initialized
    virtual void MainInitAdjustments() { };

    //called whenever the game script SetTranslation is called (so fonts can be changed, etc.)
    virtual void SetTranslation(const char* name);

    //Waits for next frame, by monitoring timerloop which is updated by an asynchronous process (or some other platform-specific mechanism)
    virtual void WaitForNextFrame();

    virtual Common::Stream* Save_CreateSlotStream(int slnum);
    //virtual void Save_DeleteSlot(int slnum);

    virtual Common::Stream *OpenFile(const Common::String &filename, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);

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

    // Gets the only platform driver instance, creates one if necessary
    static AGSPlatformDriver *GetDriver();
    // Shuts down and deletes the platform driver
    static void Shutdown();

    // Store command line arguments for the future use
    void SetCommandArgs(const char *const argv[], size_t argc);
    // Set whether PrintMessage should output to stdout or stderr
    void SetOutputToErr(bool on) { _logToStdErr = on; }
    // Set whether DisplayAlert is allowed to show modal GUIs on some systems;
    // it will print to either stdout or stderr otherwise, depending on above flag
    void SetGUIMode(bool on) { _guiMode = on; }

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
    bool _logToStdErr = false;
    // Defines whether engine is allowed to display important warnings
    // and errors by showing a message box kind of GUI.
    bool _guiMode = false;

    const char * const *_cmdArgs = nullptr;
    size_t _cmdArgCount = 0u;

private:
    static AGSPlatformDriver *CreateDriver();

    static std::unique_ptr<AGSPlatformDriver> _instance;
};

#if defined (AGS_HAS_CD_AUDIO)
int cd_player_init();
int cd_player_control(int cmdd, int datt);
#endif

// [IKM] What is a need to have this global var if you can get AGSPlatformDriver
// instance by calling AGSPlatformDriver::GetDriver()?
extern AGSPlatformDriver *platform;

#endif // __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
