/*
 * config.h: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.h 2.28 2010/09/12 11:31:21 kls Exp $
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "i18n.h"
#include "font.h"
#include "tools.h"

// VDR's own version number:

#define VDRVERSION  "1.7.16"
#define VDRVERSNUM   10716  // Version * 10000 + Major * 100 + Minor

// The plugin API's version number:

#define APIVERSION  "1.7.16"
#define APIVERSNUM   10716  // Version * 10000 + Major * 100 + Minor

// When loading plugins, VDR searches them by their APIVERSION, which
// may be smaller than VDRVERSION in case there have been no changes to
// VDR header files since the last APIVERSION. This allows compiled
// plugins to work with newer versions of the core VDR as long as no
// VDR header files have changed.

#define MAXPRIORITY 99
#define MAXLIFETIME 99

#define MINOSDWIDTH   480
#define MAXOSDWIDTH  1920
#define MINOSDHEIGHT  324
#define MAXOSDHEIGHT 1200

#define MaxFileName 256
#define MaxSkinName 16
#define MaxThemeName 16

typedef uint32_t in_addr_t; //XXX from /usr/include/netinet/in.h (apparently this is not defined on systems with glibc < 2.2)

class cSVDRPhost : public cListObject {
private:
  struct in_addr addr;
  in_addr_t mask;
public:
  cSVDRPhost(void);
  bool Parse(const char *s);
  bool IsLocalhost(void);
  bool Accepts(in_addr_t Address);
  };

template<class T> class cConfig : public cList<T> {
private:
  char *fileName;
  bool allowComments;
  void Clear(void)
  {
    free(fileName);
    fileName = NULL;
    cList<T>::Clear();
  }
public:
  cConfig(void) { fileName = NULL; }
  virtual ~cConfig() { free(fileName); }
  const char *FileName(void) { return fileName; }
  bool Load(const char *FileName = NULL, bool AllowComments = false, bool MustExist = false)
  {
    cConfig<T>::Clear();
    if (FileName) {
       free(fileName);
       fileName = strdup(FileName);
       allowComments = AllowComments;
       }
    bool result = !MustExist;
    if (fileName && access(fileName, F_OK) == 0) {
       isyslog("loading %s", fileName);
       FILE *f = fopen(fileName, "r");
       if (f) {
          char *s;
          int line = 0;
          cReadLine ReadLine;
          result = true;
          while ((s = ReadLine.Read(f)) != NULL) {
                line++;
                if (allowComments) {
                   char *p = strchr(s, '#');
                   if (p)
                      *p = 0;
                   }
                stripspace(s);
                if (!isempty(s)) {
                   T *l = new T;
                   if (l->Parse(s))
                      Add(l);
                   else {
                      esyslog("ERROR: error in %s, line %d", fileName, line);
                      delete l;
                      result = false;
                      }
                   }
                }
          fclose(f);
          }
       else {
          LOG_ERROR_STR(fileName);
          result = false;
          }
       }
    if (!result)
       fprintf(stderr, "vdr: error while reading '%s'\n", fileName);
    return result;
  }
  bool Save(void)
  {
    bool result = true;
    T *l = (T *)this->First();
    cSafeFile f(fileName);
    if (f.Open()) {
       while (l) {
             if (!l->Save(f)) {
                result = false;
                break;
                }
             l = (T *)l->Next();
             }
       if (!f.Close())
          result = false;
       }
    else
       result = false;
    return result;
  }
  };

class cNestedItem : public cListObject {
private:
  char *text;
  cList<cNestedItem> *subItems;
public:
  cNestedItem(const char *Text, bool WithSubItems = false);
  virtual ~cNestedItem();
  virtual int Compare(const cListObject &ListObject) const;
  const char *Text(void) const { return text; }
  cList<cNestedItem> *SubItems(void) { return subItems; }
  void AddSubItem(cNestedItem *Item);
  void SetText(const char *Text);
  void SetSubItems(bool On);
  };

class cNestedItemList : public cList<cNestedItem> {
private:
  char *fileName;
  bool Parse(FILE *f, cList<cNestedItem> *List, int &Line);
  bool Write(FILE *f, cList<cNestedItem> *List, int Indent = 0);
public:
  cNestedItemList(void);
  virtual ~cNestedItemList();
  void Clear(void);
  bool Load(const char *FileName);
  bool Save(void);
  };

class cSVDRPhosts : public cConfig<cSVDRPhost> {
public:
  bool LocalhostOnly(void);
  bool Acceptable(in_addr_t Address);
  };

extern cNestedItemList Folders;
extern cNestedItemList Commands;
extern cNestedItemList RecordingCommands;
extern cSVDRPhosts SVDRPhosts;

class cSetupLine : public cListObject {
private:
  char *plugin;
  char *name;
  char *value;
public:
  cSetupLine(void);
  cSetupLine(const char *Name, const char *Value, const char *Plugin = NULL);
  virtual ~cSetupLine();
  virtual int Compare(const cListObject &ListObject) const;
  const char *Plugin(void) { return plugin; }
  const char *Name(void) { return name; }
  const char *Value(void) { return value; }
  bool Parse(char *s);
  bool Save(FILE *f);
  };

class cSetup : public cConfig<cSetupLine> {
  friend class cPlugin; // needs to be able to call Store()
private:
  void StoreLanguages(const char *Name, int *Values);
  bool ParseLanguages(const char *Value, int *Values);
  bool Parse(const char *Name, const char *Value);
  cSetupLine *Get(const char *Name, const char *Plugin = NULL);
  void Store(const char *Name, const char *Value, const char *Plugin = NULL, bool AllowMultiple = false);
  void Store(const char *Name, int Value, const char *Plugin = NULL);
  void Store(const char *Name, double &Value, const char *Plugin = NULL);
public:
  // Also adjust cMenuSetup (menu.c) when adding parameters here!
  int __BeginData__;
  char OSDLanguage[I18N_MAX_LOCALE_LEN];
  char OSDSkin[MaxSkinName];
  char OSDTheme[MaxThemeName];
  int PrimaryDVB;
  int ShowInfoOnChSwitch;
  int TimeoutRequChInfo;
  int MenuScrollPage;
  int MenuScrollWrap;
  int MenuKeyCloses;
  int MarkInstantRecord;
  char NameInstantRecord[MaxFileName];
  int InstantRecordTime;
  int LnbSLOF;
  int LnbFrequLo;
  int LnbFrequHi;
  int DiSEqC;
  int SetSystemTime;
  int TimeSource;
  int TimeTransponder;
  int MarginStart, MarginStop;
  int AudioLanguages[I18N_MAX_LANGUAGES + 1];
  int DisplaySubtitles;
  int SubtitleLanguages[I18N_MAX_LANGUAGES + 1];
  int SubtitleOffset;
  int SubtitleFgTransparency, SubtitleBgTransparency;
  int EPGLanguages[I18N_MAX_LANGUAGES + 1];
  int EPGScanTimeout;
  int EPGBugfixLevel;
  int EPGLinger;
  int SVDRPTimeout;
  int ZapTimeout;
  int ChannelEntryTimeout;
  int PrimaryLimit;
  int DefaultPriority, DefaultLifetime;
  int PausePriority, PauseLifetime;
  int PauseKeyHandling;
  int UseSubtitle;
  int UseVps;
  int VpsMargin;
  int RecordingDirs;
  int FoldersInTimerMenu;
  int NumberKeysForChars;
  int VideoDisplayFormat;
  int VideoFormat;
  int UpdateChannels;
  int UseDolbyDigital;
  int ChannelInfoPos;
  int ChannelInfoTime;
  double OSDLeftP, OSDTopP, OSDWidthP, OSDHeightP;
  int OSDLeft, OSDTop, OSDWidth, OSDHeight;
  double OSDAspect;
  int OSDMessageTime;
  int UseSmallFont;
  int AntiAlias;
  char FontOsd[MAXFONTNAME];
  char FontSml[MAXFONTNAME];
  char FontFix[MAXFONTNAME];
  double FontOsdSizeP;
  double FontSmlSizeP;
  double FontFixSizeP;
  int FontOsdSize;
  int FontSmlSize;
  int FontFixSize;
  int MaxVideoFileSize;
  int SplitEditedFiles;
  int DelTimeshiftRec;
  int MinEventTimeout, MinUserInactivity;
  time_t NextWakeupTime;
  int MultiSpeedMode;
  int ShowReplayMode;
  int ResumeID;
  int CurrentChannel;
  int CurrentVolume;
  int CurrentDolby;
  int InitialChannel;
  int InitialVolume;
  int ChannelsWrap;
  int EmergencyExit;

//ML
  #define LNB_SHARING_VERSION "0.1.4"
  int VerboseLNBlog;
  #define MAXDEVICES 16 // Since VDR 1.3.32 we can not #include "device.h" for MAXDEVICES anymore.
                        // With this workaround a warning will be shown during compilation if
                        // MAXDEVICES changes in device.h.
  int CardUsesLnbNr[MAXDEVICES];
//ML-Ende

  int __EndData__;
  cSetup(void);
  cSetup& operator= (const cSetup &s);
  bool Load(const char *FileName);
  bool Save(void);
  };

extern cSetup Setup;

#endif //__CONFIG_H
