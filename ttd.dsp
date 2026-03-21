# Microsoft Developer Studio Project File - Name="ttd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=ttd - Win32 Debug Alpha
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ttd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ttd.mak" CFG="ttd - Win32 Debug Alpha"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ttd - Win32 Release Alpha" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "ttd - Win32 Debug Alpha" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "ttd - Win32 Checked Alpha" (based on "Win32 (ALPHA) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ttd - Win32 Release Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ttd___Win32_Release_Alpha"
# PROP BASE Intermediate_Dir "ttd___Win32_Release_Alpha"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ttd___Win32_Release_Alpha"
# PROP Intermediate_Dir "ttd___Win32_Release_Alpha"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /Zi /Ox /Oa /Ow /Og /Oi /Os /Gf /Gy /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "WIN32_EXCEPTION_TRACKER" /D "WIN32_ENABLE_DIRECTMUSIC_SUPPORT" /FAcs /FR /Yu"stdafx.h" /J /FD /c
# SUBTRACT BASE CPP /WX /Ot
# ADD CPP /nologo /Gt0 /W3 /Gm /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FAcs /FR /Yu"stdafx.h" /J /FD /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /map /machine:ALPHA /opt:nowin98
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /map /machine:ALPHA /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ttd - Win32 Debug Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ttd___Win32_Debug_Alpha"
# PROP BASE Intermediate_Dir "ttd___Win32_Debug_Alpha"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ttd___Win32_Debug_Alpha"
# PROP Intermediate_Dir "ttd___Win32_Debug_Alpha"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "WITH_SDL" /D "WIN32_ENABLE_DIRECTMUSIC_SUPPORT" /YX"stdafx.h" /FD /GZ /c
# SUBTRACT BASE CPP /WX /Fr
# ADD CPP /nologo /Gt0 /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "WITH_SDL" /D "WIN32_ENABLE_DIRECTMUSIC_SUPPORT" /YX"stdafx.h" /FD /GZ /c
# SUBTRACT CPP /WX /Fr
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none /incremental:no
# ADD LINK32 winmm.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "ttd - Win32 Checked Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ttd___Win32_Checked_Alpha"
# PROP BASE Intermediate_Dir "ttd___Win32_Checked_Alpha"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ttd___Win32_Checked_Alpha"
# PROP Intermediate_Dir "ttd___Win32_Checked_Alpha"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "WIN32_EXCEPTION_TRACKER" /D "WIN32_ENABLE_DIRECTMUSIC_SUPPORT" /FAcs /FR /Yu"stdafx.h" /J /FD /c
# SUBTRACT BASE CPP /WX
# ADD CPP /nologo /Gt0 /W3 /Gm /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "WIN32_EXCEPTION_TRACKER" /D "WIN32_ENABLE_DIRECTMUSIC_SUPPORT" /FAcs /FR /Yu"stdafx.h" /J /FD /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /map /machine:ALPHA /opt:nowin98
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /map /machine:ALPHA /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ttd - Win32 Release Alpha"
# Name "ttd - Win32 Debug Alpha"
# Name "ttd - Win32 Checked Alpha"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ai.c
# End Source File
# Begin Source File

SOURCE=.\command.c
# End Source File
# Begin Source File

SOURCE=.\documentation.txt
# End Source File
# Begin Source File

SOURCE=.\economy.c
# End Source File
# Begin Source File

SOURCE=.\engine.c
# End Source File
# Begin Source File

SOURCE=.\fileio.c
# End Source File
# Begin Source File

SOURCE=.\gfx.c
# End Source File
# Begin Source File

SOURCE=.\landscape.c
# End Source File
# Begin Source File

SOURCE=.\minilzo.c

!IF  "$(CFG)" == "ttd - Win32 Release Alpha"

# ADD BASE CPP /Gt0
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /Gt0
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ttd - Win32 Debug Alpha"

# ADD BASE CPP /Gt0
# SUBTRACT BASE CPP /YX
# ADD CPP /Gt0
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "ttd - Win32 Checked Alpha"

# ADD BASE CPP /Gt0
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /Gt0
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\namegen.c
# End Source File
# Begin Source File

SOURCE=.\network.c
# End Source File
# Begin Source File

SOURCE=.\oldloader.c
# End Source File
# Begin Source File

SOURCE=.\pathfind.c
# End Source File
# Begin Source File

SOURCE=.\players.c
# End Source File
# Begin Source File

SOURCE=.\saveload.c
# End Source File
# Begin Source File

SOURCE=.\screenshot.c
# End Source File
# Begin Source File

SOURCE=.\sdl.c
# End Source File
# Begin Source File

SOURCE=.\settings.c
# End Source File
# Begin Source File

SOURCE=.\sound.c
# End Source File
# Begin Source File

SOURCE=.\spritecache.c
# End Source File
# Begin Source File

SOURCE=.\StdAfx.c

!IF  "$(CFG)" == "ttd - Win32 Release Alpha"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ttd - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "ttd - Win32 Checked Alpha"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\strings.c
# End Source File
# Begin Source File

SOURCE=.\texteff.c
# End Source File
# Begin Source File

SOURCE=.\ttd.c
# End Source File
# Begin Source File

SOURCE=.\ttd.rc
# End Source File
# Begin Source File

SOURCE=.\unix.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vehicle.c
# End Source File
# Begin Source File

SOURCE=.\viewport.c
# End Source File
# Begin Source File

SOURCE=.\w32dm.c
# End Source File
# Begin Source File

SOURCE=.\w32dm2.cpp

!IF  "$(CFG)" == "ttd - Win32 Release Alpha"

# ADD BASE CPP /Gt0
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /Gt0
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ttd - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "ttd - Win32 Checked Alpha"

# ADD BASE CPP /Gt0
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /Gt0
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\widget.c
# End Source File
# Begin Source File

SOURCE=.\win32.c
# End Source File
# Begin Source File

SOURCE=.\window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\city.h
# End Source File
# Begin Source File

SOURCE=.\command.h
# End Source File
# Begin Source File

SOURCE=.\economy.h
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=.\fileio.h
# End Source File
# Begin Source File

SOURCE=.\functions.h
# End Source File
# Begin Source File

SOURCE=.\gfx.h
# End Source File
# Begin Source File

SOURCE=.\gui.h
# End Source File
# Begin Source File

SOURCE=.\hal.h
# End Source File
# Begin Source File

SOURCE=.\industry.h
# End Source File
# Begin Source File

SOURCE=.\macros.h
# End Source File
# Begin Source File

SOURCE=.\news.h
# End Source File
# Begin Source File

SOURCE=.\pathfind.h
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\saveload.h
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\station.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ttd.h
# End Source File
# Begin Source File

SOURCE=.\variables.h
# End Source File
# Begin Source File

SOURCE=.\vehicle.h
# End Source File
# Begin Source File

SOURCE=.\viewport.h
# End Source File
# Begin Source File

SOURCE=.\window.h
# End Source File
# End Group
# Begin Group "Listings"

# PROP Default_Filter "*.cod"
# Begin Source File

SOURCE=.\Release\ai.cod
# End Source File
# Begin Source File

SOURCE=.\Release\aircraft_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\clear_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\command.cod
# End Source File
# Begin Source File

SOURCE=.\Release\depot_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\disaster_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\economy.cod
# End Source File
# Begin Source File

SOURCE=.\Release\engine.cod
# End Source File
# Begin Source File

SOURCE=.\Release\gfx.cod
# End Source File
# Begin Source File

SOURCE=.\Release\goods.cod
# End Source File
# Begin Source File

SOURCE=.\Release\graph_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\industry_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\industry_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\landscape.cod
# End Source File
# Begin Source File

SOURCE=.\Release\main_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\minilzo.cod
# End Source File
# Begin Source File

SOURCE=.\Release\misc.cod
# End Source File
# Begin Source File

SOURCE=.\Release\misc_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\news_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\player_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\player_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\players.cod
# End Source File
# Begin Source File

SOURCE=.\Release\rail_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\road_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\saveload.cod
# End Source File
# Begin Source File

SOURCE=.\Release\settings_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\ship_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\smallmap_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\sound.cod
# End Source File
# Begin Source File

SOURCE=.\Release\spritecache.cod
# End Source File
# Begin Source File

SOURCE=.\Release\station_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\strings.cod
# End Source File
# Begin Source File

SOURCE=.\Release\town_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\town_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\train_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\train_gui.cod
# End Source File
# Begin Source File

SOURCE=.\Release\tree_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\ttd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\vehicle.cod
# End Source File
# Begin Source File

SOURCE=.\Release\viewport.cod
# End Source File
# Begin Source File

SOURCE=.\Release\water_cmd.cod
# End Source File
# Begin Source File

SOURCE=.\Release\widget.cod
# End Source File
# Begin Source File

SOURCE=.\Release\win32.cod
# End Source File
# Begin Source File

SOURCE=.\Release\window.cod
# End Source File
# End Group
# Begin Group "Gui Source codes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\aircraft_gui.c
# End Source File
# Begin Source File

SOURCE=.\airport_gui.c
# End Source File
# Begin Source File

SOURCE=.\bridge_gui.c
# End Source File
# Begin Source File

SOURCE=.\dock_gui.c
# End Source File
# Begin Source File

SOURCE=.\engine_gui.c
# End Source File
# Begin Source File

SOURCE=.\graph_gui.c
# End Source File
# Begin Source File

SOURCE=.\industry_gui.c
# End Source File
# Begin Source File

SOURCE=.\intro_gui.c
# End Source File
# Begin Source File

SOURCE=.\main_gui.c
# End Source File
# Begin Source File

SOURCE=.\misc_gui.c
# End Source File
# Begin Source File

SOURCE=.\music_gui.c
# End Source File
# Begin Source File

SOURCE=.\news_gui.c
# End Source File
# Begin Source File

SOURCE=.\order_gui.c
# End Source File
# Begin Source File

SOURCE=.\player_gui.c
# End Source File
# Begin Source File

SOURCE=.\rail_gui.c
# End Source File
# Begin Source File

SOURCE=.\road_gui.c
# End Source File
# Begin Source File

SOURCE=.\roadveh_gui.c
# End Source File
# Begin Source File

SOURCE=.\settings_gui.c
# End Source File
# Begin Source File

SOURCE=.\ship_gui.c
# End Source File
# Begin Source File

SOURCE=.\smallmap_gui.c
# End Source File
# Begin Source File

SOURCE=.\station_gui.c
# End Source File
# Begin Source File

SOURCE=.\subsidy_gui.c
# End Source File
# Begin Source File

SOURCE=.\town_gui.c
# End Source File
# Begin Source File

SOURCE=.\train_gui.c
# End Source File
# End Group
# Begin Group "Landscape"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\aircraft_cmd.c
# End Source File
# Begin Source File

SOURCE=.\clear_cmd.c
# End Source File
# Begin Source File

SOURCE=.\disaster_cmd.c
# End Source File
# Begin Source File

SOURCE=.\dummy_land.c
# End Source File
# Begin Source File

SOURCE=.\industry_cmd.c
# End Source File
# Begin Source File

SOURCE=.\misc_cmd.c
# End Source File
# Begin Source File

SOURCE=.\order_cmd.c
# End Source File
# Begin Source File

SOURCE=.\rail_cmd.c
# End Source File
# Begin Source File

SOURCE=.\road_cmd.c
# End Source File
# Begin Source File

SOURCE=.\roadveh_cmd.c
# End Source File
# Begin Source File

SOURCE=.\ship_cmd.c
# End Source File
# Begin Source File

SOURCE=.\station_cmd.c
# End Source File
# Begin Source File

SOURCE=.\town_cmd.c
# End Source File
# Begin Source File

SOURCE=.\train_cmd.c
# End Source File
# Begin Source File

SOURCE=.\tree_cmd.c
# End Source File
# Begin Source File

SOURCE=.\tunnelbridge_cmd.c
# End Source File
# Begin Source File

SOURCE=.\unmovable_cmd.c
# End Source File
# Begin Source File

SOURCE=.\water_cmd.c
# End Source File
# End Group
# Begin Group "Tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\table\ai_rail.h
# End Source File
# Begin Source File

SOURCE=.\table\allstrings.h
# End Source File
# Begin Source File

SOURCE=.\table\animcursors.h
# End Source File
# Begin Source File

SOURCE=.\table\build_industry.h
# End Source File
# Begin Source File

SOURCE=.\table\city_land.h
# End Source File
# Begin Source File

SOURCE=.\table\clear_land.h
# End Source File
# Begin Source File

SOURCE=.\table\engines.h
# End Source File
# Begin Source File

SOURCE=.\table\genland.h
# End Source File
# Begin Source File

SOURCE=.\table\industry_land.h
# End Source File
# Begin Source File

SOURCE=.\table\landscape_const.h
# End Source File
# Begin Source File

SOURCE=.\table\landscape_sprite.h
# End Source File
# Begin Source File

SOURCE=.\table\palettes.h
# End Source File
# Begin Source File

SOURCE=.\table\road_land.h
# End Source File
# Begin Source File

SOURCE=.\table\roadveh.h
# End Source File
# Begin Source File

SOURCE=.\table\station_land.h
# End Source File
# Begin Source File

SOURCE=.\table\strings.h
# End Source File
# Begin Source File

SOURCE=.\table\track_land.h
# End Source File
# Begin Source File

SOURCE=.\table\train_cmd.h
# End Source File
# Begin Source File

SOURCE=.\table\tree_land.h
# End Source File
# Begin Source File

SOURCE=.\table\tunnel_land.h
# End Source File
# Begin Source File

SOURCE=.\table\unmovable_land.h
# End Source File
# Begin Source File

SOURCE=.\table\water_land.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\changelog.txt
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\mainicon.ico
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
