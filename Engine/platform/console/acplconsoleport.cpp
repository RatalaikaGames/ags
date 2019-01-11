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

//At the last minute I decided to call this the "console" driver.
//The hope is that no matter how you implement your console backend, the interface to AGS will be similar
//So I piped everything through here in an effort to help other people do it.
//However, for now, this is just the "rata" console

#include <stdio.h>
#include <allegro.h>
#include "ac/runtime_defines.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "util/string.h"
#include <libcda.h>

using AGS::Common::String;

struct AGSConsolePort : AGSPlatformDriver {

  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual const char *GetUserSavedgamesDirectory();
  virtual const char *GetUserConfigDirectory();
  virtual const char *GetUserGlobalConfigDirectory();
  virtual const char *GetAppOutputDirectory();
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetNoMouseErrorString();
  virtual bool IsMouseControlSupported(bool windowed);
  virtual const char* GetAllegroFailUserHint();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void PlayVideo(const char* name, int skip, int flags);
  virtual void PostAllegroExit();
  virtual void SetGameWindowIcon();
  virtual void ShutdownCDPlayer();
  virtual bool LockMouseToWindow();
  virtual void UnlockMouse();
};


int AGSConsolePort::CDPlayerCommand(int cmdd, int datt) {
  return 0;
}

void AGSConsolePort::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
}

size_t BuildXDGPath(char *destPath, size_t destSize)
{
	return 0;
  //// Check to see if XDG_DATA_HOME is set in the enviroment
  //const char* home_dir = getenv("XDG_DATA_HOME");
  //size_t l = 0;
  //if (home_dir)
  //{
  //  l = snprintf(destPath, destSize, "%s", home_dir);
  //}
  //else
  //{
  //  // No evironment variable, so we fall back to home dir in /etc/passwd
  //  struct passwd *p = getpwuid(getuid());
  //  l = snprintf(destPath, destSize, "%s/.local", p->pw_dir);
  //  if (mkdir(destPath, 0755) != 0 && errno != EEXIST)
  //    return 0;
  //  l += snprintf(destPath + l, destSize - l, "/share");
  //  if (mkdir(destPath, 0755) != 0 && errno != EEXIST)
  //    return 0;
  //}
  //l += snprintf(destPath + l, destSize - l, "/ags");
  //if (mkdir(destPath, 0755) != 0 && errno != EEXIST)
  //  return 0;
  //return l;
}

void DetermineAppOutputDirectory()
{
  //if (!LinuxOutputDirectory.IsEmpty())
  //{
  //  return;
  //}

  //char xdg_path[256];
  //if (BuildXDGPath(xdg_path, sizeof(xdg_path)) > 0)
  //  LinuxOutputDirectory = xdg_path;
  //else
  //  LinuxOutputDirectory = "/tmp";
}

const char *AGSConsolePort::GetUserSavedgamesDirectory()
{
	return "";
  //DetermineAppOutputDirectory();
  //return LinuxOutputDirectory;
}

const char *AGSConsolePort::GetUserConfigDirectory()
{
  return GetUserSavedgamesDirectory();
}

const char *AGSConsolePort::GetUserGlobalConfigDirectory()
{
  return GetUserSavedgamesDirectory();
}

const char *AGSConsolePort::GetAppOutputDirectory()
{
  DetermineAppOutputDirectory();
  //return LinuxOutputDirectory;
	return "";
}

void AGSConsolePort::Delay(int millis) {
  //struct timespec ts;
  //ts.tv_sec = 0;
  //ts.tv_nsec = millis * 1000000;
  //nanosleep(&ts, NULL);
	//TODO
}

unsigned long AGSConsolePort::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSConsolePort::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

bool AGSConsolePort::IsMouseControlSupported(bool windowed)
{
  return true; // supported for both fullscreen and windowed modes
}

const char* AGSConsolePort::GetAllegroFailUserHint()
{
  return "Make sure you have latest version of Allegro 4 libraries installed, and X server is running.";
}

eScriptSystemOSID AGSConsolePort::GetSystemOSID() {
  return eOS_Linux;
}

int AGSConsolePort::InitializeCDPlayer() {
  //return cd_player_init();
	return 0;
}

void AGSConsolePort::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSConsolePort::PostAllegroExit() {
  // do nothing
}

void AGSConsolePort::SetGameWindowIcon() {
  // do nothing
}

void AGSConsolePort::ShutdownCDPlayer() {
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSConsolePort();
  return instance;
}

bool AGSConsolePort::LockMouseToWindow()
{
	return false;
}

void AGSConsolePort::UnlockMouse()
{
    //XUngrabPointer(_xwin.display, CurrentTime);
}
