; Define your application name
!define APPNAME "OpenTTD"
!define APPNAMEANDVERSION "OpenTTD 0.2.0"
!define APPVERSION "0.2.0"
BrandingText "OpenTTD Installer"


; Version Info
Var AddWinPrePopulate
VIProductVersion "0.2.0.0"
VIAddVersionKey "ProductName" "OpenTTD Installer"
VIAddVersionKey "Comments" "Installs ${APPNAMEANDVERSION}"
VIAddVersionKey "CompanyName" "OpenTTD Developers"
VIAddVersionKey "FileDescription" "Installs ${APPNAMEANDVERSION}"
VIAddVersionKey "ProductVersion" "${APPVERSION}"
VIAddVersionKey "InternalName" "InstOpenTTD"
VIAddVersionKey "FileVersion" "${APPVERSION}"
VIAddVersionKey "LegalCopyright" " "
; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES\OpenTTD\"
InstallDirRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Install Folder"
OutFile "openttd-${APPVERSION}.exe"
ShowInstDetails show
ShowUninstDetails show
SetCompressor LZMA

Var SHORTCUTS

; Modern interface settings
!include "MUI.nsh"

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
LicenseForceSelection radiobuttons "I &accept this Agreement" "I &do not accept this agreement"

!insertmacro MUI_PAGE_LICENSE "gpl.txt"
;--------------------------------
; Special page for finding the CD
Var CDDRIVE
; The page itself
PageEx directory

	!define MUI_DIRECTORYPAGE_VARIABLE $INSTDIR
	DirVar $CDDRIVE
	PageCallbacks CDInit "" checkCD
	DirText "$AddWinPrePopulate$\n" "Transport Tycoon Deluxe Installation Location" "" "Folder where Transport Tycoon Deluxe is located:"
PageExEnd

; Check for CD
Function CDInit
  !insertmacro MUI_HEADER_TEXT "Locate TTD" "Setup needs the location of Transport Tycoon Deluxe in order to continue."
FunctionEnd

; Required to select CD
AllowRootDirInstall true ;But doesn't work: There's no "free space" on the CD
Function checkCD
; StrCmp
 IfFileExists $CDDRIVE\trg1r.grf "" NoCD
 IfFileExists $CDDRIVE\sample.cat "" NoCD
 IfFileExists $CDDRIVE\trgir.grf hasCD ""
NoCD:
  MessageBox MB_OK "Setup cannot continue without the Transport Tycoon Deluxe Location!"
  Abort
hasCD:
FunctionEnd
;--------------------------------
; Rest of pages
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

;Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_DEFAULTFOLDER $SHORTCUTS
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKEY_LOCAL_MACHINE"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Shortcut Folder"

!insertmacro MUI_PAGE_STARTMENU "OpenTTD" $SHORTCUTS

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\ttd.exe"
!define MUI_FINISHPAGE_LINK "Visit OpenTTD's homepage"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.sf.net/projects/openttd"
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\readme.txt"

!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set languages (first is default language)
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
; Sections
Section "OpenTTD" Section1
;Since this section is first, I'm going to locate and remove older versions (0.1.3 and below)

	; Set Section properties
	SetOverwrite try

	;Looks for proof of older version and uninstalls it first
	;Once done, Install will proceed like normal
	;This only removes version 0.1.3 and below (as this is to clean up RSSW 2.x based installations)
	DetailPrint "Setup is removing the previous version..."
	Delete "$INSTDIR\compact\*.*"
	Delete "$INSTDIR\*.*"
	RMDir /r "$INSTDIR\compact"
	DetailPrint "Removal Complete"
	DetailPrint "Starting ${APPNAMEANDVERSION} Installation..."
	Sleep 1000

	; Set Section Files and Shortcuts
	SetOutPath "$INSTDIR\"

	;Install OpenTTD
  File "..\changelog.txt"
  File "..\lang\english.lng"
;  File "..\lang\french.lng"
  File "..\lang\german.lng"
  File "..\lang\italian.lng"
  File "opntitle.dat"
  File "..\readme.txt"
  File "signalsw.grf"
  File "openttd.grf"
  File "..\Release\ttd.exe"
  File "..\Release\ttd.map"

	;Creates the Registry Entries
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Comments" "Visit http://www.tt-forums.net"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "DisplayIcon" "$INSTDIR\setup.ico"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "DisplayName" "OpenTTD ${APPVERSION}"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "DisplayVersion" "${APPVERSION}"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "HelpLink" "http://www.tt-forums.net"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Install Folder" "$INSTDIR"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Publisher" "OpenTTD"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Shortcut Folder" "$SHORTCUTS"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "URLInfoAbout" "http://www.sf.net/projects/openttd"
	;This key sets the Version DWORD that patches will check against
	WriteRegDWORD HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Version" 6
  
	CreateShortCut "$DESKTOP\OpenTTD.lnk" "$INSTDIR\ttd.exe"
	CreateDirectory "$SMPROGRAMS\$SHORTCUTS"
	CreateDirectory "$SMPROGRAMS\$SHORTCUTS\Extras"
	CreateShortCut "$SMPROGRAMS\$SHORTCUTS\OpenTTD.lnk" "$INSTDIR\ttd.exe"
	CreateShortCut "$SMPROGRAMS\$SHORTCUTS\Uninstall.lnk" "$INSTDIR\uninstall.exe"
	CreateShortCut "$SMPROGRAMS\$SHORTCUTS\Extras\OpenTTD Changelog.lnk" "$INSTDIR\Changelog.txt"
SectionEnd

Section "Copy Game Graphics" Section2
	; Include files from CD
	;Let's copy the files from the CD
	CreateDirectory "$INSTDIR\gm"
	SetOutPath "$INSTDIR\gm"
	CopyFiles "$CDDRIVE\gm\gm_tt00.gm" "$INSTDIR\gm\gm_tt00.gm" 29
	CopyFiles "$CDDRIVE\gm\gm_tt01.gm" "$INSTDIR\gm\gm_tt01.gm" 49
	CopyFiles "$CDDRIVE\gm\gm_tt02.gm" "$INSTDIR\gm\gm_tt02.gm" 45
	CopyFiles "$CDDRIVE\gm\gm_tt03.gm" "$INSTDIR\gm\gm_tt03.gm" 59
	CopyFiles "$CDDRIVE\gm\gm_tt04.gm" "$INSTDIR\gm\gm_tt04.gm" 61
	CopyFiles "$CDDRIVE\gm\gm_tt05.gm" "$INSTDIR\gm\gm_tt05.gm" 61
	CopyFiles "$CDDRIVE\gm\gm_tt06.gm" "$INSTDIR\gm\gm_tt06.gm" 67
	CopyFiles "$CDDRIVE\gm\gm_tt07.gm" "$INSTDIR\gm\gm_tt07.gm" 36
	CopyFiles "$CDDRIVE\gm\gm_tt08.gm" "$INSTDIR\gm\gm_tt08.gm" 17
	CopyFiles "$CDDRIVE\gm\gm_tt09.gm" "$INSTDIR\gm\gm_tt09.gm" 56
	CopyFiles "$CDDRIVE\gm\gm_tt10.gm" "$INSTDIR\gm\gm_tt10.gm" 42
	CopyFiles "$CDDRIVE\gm\gm_tt11.gm" "$INSTDIR\gm\gm_tt11.gm" 56
	CopyFiles "$CDDRIVE\gm\gm_tt12.gm" "$INSTDIR\gm\gm_tt12.gm" 39
	CopyFiles "$CDDRIVE\gm\gm_tt13.gm" "$INSTDIR\gm\gm_tt13.gm" 34
	CopyFiles "$CDDRIVE\gm\gm_tt14.gm" "$INSTDIR\gm\gm_tt14.gm" 14
	CopyFiles "$CDDRIVE\gm\gm_tt15.gm" "$INSTDIR\gm\gm_tt15.gm" 59
	CopyFiles "$CDDRIVE\gm\gm_tt16.gm" "$INSTDIR\gm\gm_tt16.gm" 47
	CopyFiles "$CDDRIVE\gm\gm_tt17.gm" "$INSTDIR\gm\gm_tt17.gm" 42
	CopyFiles "$CDDRIVE\gm\gm_tt18.gm" "$INSTDIR\gm\gm_tt18.gm" 50
	CopyFiles "$CDDRIVE\gm\gm_tt19.gm" "$INSTDIR\gm\gm_tt19.gm" 49
	CopyFiles "$CDDRIVE\gm\gm_tt20.gm" "$INSTDIR\gm\gm_tt20.gm" 62
	CopyFiles "$CDDRIVE\gm\gm_tt21.gm" "$INSTDIR\gm\gm_tt21.gm" 45
	SetOutPath "$INSTDIR\"
	CopyFiles "$CDDRIVE\sample.cat" "$INSTDIR\sample.cat" 1566
	CopyFiles "$CDDRIVE\trg1r.grf" "$INSTDIR\trg1r.grf" 2365
	CopyFiles "$CDDRIVE\trgcr.grf" "$INSTDIR\trgcr.grf" 260
	CopyFiles "$CDDRIVE\trghr.grf" "$INSTDIR\trghr.grf" 400
	CopyFiles "$CDDRIVE\trgir.grf" "$INSTDIR\trgir.grf" 334
	CopyFiles "$CDDRIVE\trgtr.grf" "$INSTDIR\trgtr.grf" 546
SectionEnd

;----------------------
Section "Saved Game Folder" Section3
	CreateDirectory "$INSTDIR\Save"
	SetOutPath "$INSTDIR\Save"
SectionEnd

Section "Transition Guide" Section4
		SetOutPath "$INSTDIR\"
		File "TTD to OpenTTD Transition Guide.txt"
		CreateShortCut "$SMPROGRAMS\$SHORTCUTS\Extras\TTD to OpenTTD Transition Guide.lnk" "$INSTDIR\TTD to OpenTTD Transition Guide.txt"
SectionEnd

Section -FinishSection
	WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; Modern install component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Section1} "OpenTTD is a fully functional clone of TTD and is very playable."
	!insertmacro MUI_DESCRIPTION_TEXT ${Section2} "Copies the game graphics."
	!insertmacro MUI_DESCRIPTION_TEXT ${Section3} "This creates a Save folder for your saved games."
	!insertmacro MUI_DESCRIPTION_TEXT ${Section4} "TTD to OpenTTD Transition Guide allows you to understand the differences between TTD and OpenTTD.$\n$\nSubmit your ideas for it to TallwoodBand@cox.net"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;!undef SHORTCUTS
;Uninstall section
Section Uninstall
	MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION "Would you like to remove the Saved Game Folder located at '$INSTDIR\Save?'  If you choose Yes, your Saved Games will be removed." IDYES RemoveSavedGames IDNO DoUninstall
RemoveSavedGames:
	Delete "$INSTDIR\Save\*.*"
	RMDir "$INSTDIR\Save"
	
DoUninstall:
	;Remove from registry...
	!insertmacro MUI_STARTMENU_GETFOLDER "OpenTTD" $SHORTCUTS
	ReadRegStr $SHORTCUTS HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Shortcut Folder"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD"

	; Delete self
	Delete "$INSTDIR\uninstall.exe"

	; Delete Shortcuts
	Delete "$DESKTOP\OpenTTD.lnk"
	Delete "$SMPROGRAMS\$SHORTCUTS\OpenTTD.lnk"
	Delete "$SMPROGRAMS\$SHORTCUTS\Uninstall.lnk"
	Delete "$SMPROGRAMS\$SHORTCUTS\Check for Updates.lnk"
	Delete "$SMPROGRAMS\$SHORTCUTS\Extras\OpenTTD Changelog.lnk"
	Delete "$SMPROGRAMS\$SHORTCUTS\Extras\OpenTTD Installer Changelog.lnk"
	Delete "$SMPROGRAMS\$SHORTCUTS\Extras\TTD to OpenTTD Transition Guide.lnk"

	; Clean up OpenTTD
	Delete "$INSTDIR\changelog.txt"
	Delete "$INSTDIR\english.lng"
;	Delete "$INSTDIR\french.lng"
	Delete "$INSTDIR\german.lng"
	Delete "$INSTDIR\italian.lng"
	Delete "$INSTDIR\opntitle.dat"
	Delete "$INSTDIR\readme.txt"
	Delete "$INSTDIR\signalsw.grf"
	Delete "$INSTDIR\openttd.grf"
	Delete "$INSTDIR\ttd.exe"
	Delete "$INSTDIR\ttd.map"
	Delete "$INSTDIR\INSTALL.LOG"
	Delete "$INSTDIR\TTD to OpenTTD Transition Guide.txt"

	Delete "$INSTDIR\trg1r.grf"
	Delete "$INSTDIR\trghr.grf"
	Delete "$INSTDIR\trgtr.grf"
	Delete "$INSTDIR\sample.cat"
	Delete "$INSTDIR\trgcr.grf"
	Delete "$INSTDIR\trgir.grf"
	;Music
	Delete "$INSTDIR\gm\*.*"
	Delete "$INSTDIR\*.*"

	; Remove remaining directories
	RMDir "$SMPROGRAMS\$SHORTCUTS\Extras\"
	RMDir "$SMPROGRAMS\$SHORTCUTS"
	RMDir "$INSTDIR\gm"
	RMDir "$INSTDIR\" 
SectionEnd


Var OLDVERSION
Var UninstallString

Function .onInit
	StrCpy $SHORTCUTS "OpenTTD"
	
	;Want to have a splash BMP?  Uncomment these lines - CAREFUL WITH FILE SIZE
	
;	        # the plugins dir is automatically deleted when the installer exits
;        InitPluginsDir
;				File /oname=$PLUGINSDIR\splash.bmp "C:\Documents and Settings\Administrator\My Documents\My Pictures\OpenTTD Splash.bmp"
;        #optional
;        #File /oname=$PLUGINSDIR\splash.wav "C:\myprog\sound.wav"
;
;        ;MessageBox MB_OK "Fading"
;
;       advsplash::show 3000 600 400 -1 $PLUGINSDIR\splash
;
;      Pop $0          ; $0 has '1' if the user closed the splash screen early,
;                        ; '0' if everything closed normal, and '-1' if some error occured.
;End Splash Area
		;Starts Setup - let's look for an older version of OpenTTD
	ReadRegDWORD $R8 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Version"

	IfErrors ShowWelcomeMessage ShowUpgradeMessage
ShowWelcomeMessage:
	ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "Version"
	;In the event someone still has OpenTTD 0.1, this will detect that (that installer used a string instead of dword entry)
	IfErrors FinishCallback

ShowUpgradeMessage:
	IntCmp 6 $R8 VersionsAreEqual InstallerIsOlder  WelcomeToSetup
WelcomeToSetup:
	;An older version was found.  Let's let the user know there's an upgrade that will take plce.
	ReadRegStr $OLDVERSION HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "DisplayVersion"
	;Gets the older version then displays it in a message box
	MessageBox MB_OK|MB_ICONINFORMATION "Welcome to ${APPNAMEANDVERSION} Setup.$\n$\nThis will allow you to upgrade from version $OLDVERSION."
	Goto FinishCallback

VersionsAreEqual:
	ReadRegStr $UninstallString HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenTTD" "UninstallString"
	MessageBox MB_YESNO|MB_ICONQUESTION "Setup detected ${APPNAMEANDVERSION} on your system. That's the version this program will install.$\n$\nAre you trying to uninstall it?" IDYES DoUninstall IDNO FinishCallback
DoUninstall: ;You have the same version as this installer.  This allows you to uninstall.
	Exec "$UninstallString"
	Quit

InstallerIsOlder:
	MessageBox MB_OK|MB_ICONSTOP "You have a newer version of ${APPNAME}.$\n$\nSetup will now exit."
	Quit

FinishCallback:
	ClearErrors
	;Now, let's populate $CDDRIVE
	ReadRegStr $CDDRIVE HKLM "SOFTWARE\Fish Technology Group\Transport Tycoon Deluxe" "HDPath"
	IfErrors NoTTD
		StrCpy "$AddWinPrePopulate" "Setup has detected your TTD folder.  Don't change the folder.  Simply press Next."
		Goto TruFinish
	NoTTD:
		;Ok, let's populate it with the DOS Folder (C:\MPS\TTDLX)
		StrCpy "$CDDRIVE" "C:\MPS\TTDLX"
		StrCpy "$AddWinPrePopulate" "Setup couldn't find TTD.  Therefore, I used the common DOS version install location.  Once you correct (if needed) the path, press Next to continue."
	TruFinish:
	ClearErrors
FunctionEnd
; eof

