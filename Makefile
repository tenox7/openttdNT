all: ttd.exe

.c.obj:
	cl /nologo /O2 /Zi /J /DWIN32 /DNDEBUG /c $<

ttd.res: ttd.rc resource.h
	rc /dNDEBUG ttd.rc

ttd.exe: ai.obj aircraft_cmd.obj aircraft_gui.obj airport_gui.obj \
	bridge_gui.obj clear_cmd.obj command.obj disaster_cmd.obj \
	dock_gui.obj dummy_land.obj economy.obj engine.obj engine_gui.obj \
	fileio.obj gfx.obj graph_gui.obj industry_cmd.obj industry_gui.obj \
	intro_gui.obj landscape.obj main_gui.obj minilzo.obj misc.obj \
	misc_cmd.obj misc_gui.obj music_gui.obj namegen.obj network.obj \
	news_gui.obj oldloader.obj order_cmd.obj order_gui.obj pathfind.obj \
	player_gui.obj players.obj rail_cmd.obj rail_gui.obj road_cmd.obj \
	road_gui.obj roadveh_cmd.obj roadveh_gui.obj saveload.obj \
	screenshot.obj settings.obj settings_gui.obj ship_cmd.obj \
	ship_gui.obj smallmap_gui.obj sound.obj spritecache.obj \
	station_cmd.obj station_gui.obj strings.obj subsidy_gui.obj \
	texteff.obj town_cmd.obj town_gui.obj train_cmd.obj train_gui.obj \
	tree_cmd.obj ttd.obj tunnelbridge_cmd.obj unmovable_cmd.obj \
	vehicle.obj viewport.obj water_cmd.obj widget.obj win32.obj \
	window.obj ttd.res
	link /nologo /subsystem:windows /debug /out:ttd.exe \
	ai.obj aircraft_cmd.obj aircraft_gui.obj airport_gui.obj \
	bridge_gui.obj clear_cmd.obj command.obj disaster_cmd.obj \
	dock_gui.obj dummy_land.obj economy.obj engine.obj engine_gui.obj \
	fileio.obj gfx.obj graph_gui.obj industry_cmd.obj industry_gui.obj \
	intro_gui.obj landscape.obj main_gui.obj minilzo.obj misc.obj \
	misc_cmd.obj misc_gui.obj music_gui.obj namegen.obj network.obj \
	news_gui.obj oldloader.obj order_cmd.obj order_gui.obj pathfind.obj \
	player_gui.obj players.obj rail_cmd.obj rail_gui.obj road_cmd.obj \
	road_gui.obj roadveh_cmd.obj roadveh_gui.obj saveload.obj \
	screenshot.obj settings.obj settings_gui.obj ship_cmd.obj \
	ship_gui.obj smallmap_gui.obj sound.obj spritecache.obj \
	station_cmd.obj station_gui.obj strings.obj subsidy_gui.obj \
	texteff.obj town_cmd.obj town_gui.obj train_cmd.obj train_gui.obj \
	tree_cmd.obj ttd.obj tunnelbridge_cmd.obj unmovable_cmd.obj \
	vehicle.obj viewport.obj water_cmd.obj widget.obj win32.obj \
	window.obj ttd.res \
	winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib \
	shell32.lib comdlg32.lib

clean:
	-del *.obj
	-del ttd.exe
	-del ttd.res
