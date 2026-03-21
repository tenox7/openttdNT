#include "stdafx.h"
#include "ttd.h"
#include "hal.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#if defined(__linux__)
#include <sys/statvfs.h>
#endif

#if defined(WITH_SDL)
#include <SDL.h>
#endif

int rdtsc()
{
	return 0;
}

static char *_fios_path;
static FiosItem *_fios_items;
static int _fios_count, _fios_alloc;

static FiosItem *FiosAlloc()
{
	if (_fios_count == _fios_alloc) {
		_fios_alloc += 256;
		_fios_items = realloc(_fios_items, _fios_alloc * sizeof(FiosItem));
	}
	return &_fios_items[_fios_count++];
}

static int compare_FiosItems (const void *a, const void *b) {
	const FiosItem *da = (const FiosItem *) a;
	const FiosItem *db = (const FiosItem *) b;
	return strcmp(da->name, db->name);
}


// Get a list of savegames
const FiosItem *FiosGetSavegameList(int *num, int mode)
{
	FiosItem *fios;
	DIR *dir;
	struct dirent *dirent;
	struct stat sb;
	char filename[MAX_PATH];

	if (_fios_path == NULL) {
		_fios_path = malloc(MAX_PATH);
		getcwd(_fios_path, MAX_PATH);
	}

	// Parent directory, only if not in root already.
	if (_fios_path[1] != 0) {
		fios = FiosAlloc();
		fios->type = FIOS_TYPE_PARENT;
		sprintf(fios->name, ".. (Parent directory)");
	}

	dir = opendir(_fios_path[0] ? _fios_path : "/");
	if (dir != NULL) {
		while ((dirent = readdir(dir))) {
			sprintf (filename, "%s/%s", _fios_path, dirent->d_name);
			if (!lstat(filename, &sb)) {

			/* lstat does this for us
			// check if it's a symlink to a directory
			if (dirent->d_type == DT_LNK) {
				char buf[MAX_PATH];
				sprintf(buf, "%s/%s", _fios_path, dirent->d_name);
				if (stat(buf, &sb) != -1 && sb.st_mode & S_IFDIR)
					dirent->d_type = DT_DIR;
			}*/

				if (S_ISDIR(sb.st_mode)) {
					if (dirent->d_name[0] != '.') {
						fios = FiosAlloc();
						fios->type = FIOS_TYPE_DIR;
						sprintf(fios->name, "%s/ (Directory)", dirent->d_name);

					}
				} else {
					char *t = strrchr(dirent->d_name, '.');
					if (t && !strcmp(t, ".sav")) {
						*t = 0;
						fios = FiosAlloc();
						fios->type = FIOS_TYPE_FILE;
						ttd_strlcpy(fios->name, dirent->d_name, sizeof(fios->name));
					} else if (t && !strcmp(t, ".sv1")) {
						*t = 0;
						fios = FiosAlloc();
						fios->type = FIOS_TYPE_OLDFILE;
						ttd_strlcpy(fios->name, dirent->d_name, sizeof(fios->name));
					}
				}
			}
		}

		closedir(dir);
	}

	*num = _fios_count;
	qsort(_fios_items, _fios_count, sizeof(FiosItem), compare_FiosItems);
	return _fios_items;
}

// Free the list of savegames
void FiosFreeSavegameList()
{
	free(_fios_items);
	_fios_items = NULL;
	_fios_alloc = _fios_count = 0;
}

// Browse to
char *FiosBrowseTo(const FiosItem *item)
{
	char *path = _fios_path;
	char *s;

	switch(item->type) {
	case FIOS_TYPE_PARENT:
		s = strrchr(path, '/');
		if (s != NULL) *s = 0;
		break;

	case FIOS_TYPE_DIR:
		s = strchr((char*)item->name, '/');
		if (s) *s = 0;
		while (*path) path++;
		*path++ = '/';
		strcpy(path, item->name);
		break;

	case FIOS_TYPE_FILE:
		FiosMakeSavegameName(str_buffr, item->name);
		return str_buffr;

	case FIOS_TYPE_OLDFILE:
		sprintf(str_buffr, "%s/%s.sv1", path, item->name);
		return str_buffr;
	}

	return NULL;
}

// Get descriptive texts.
// Returns a path as well as a
//  string describing the path.
StringID FiosGetDescText(char **path)
{
	*path = _fios_path[0] ? _fios_path : "/";

#if defined(__linux__)
	{
	struct statvfs s;

	if (statvfs(*path, &s) == 0)
	{
		uint64 tot = (uint64)s.f_bsize * s.f_bavail;
		SET_DPARAM32(0, (uint32)(tot >> 20));
		return STR_4005_BYTES_FREE; 
	}
	else
		return STR_4006_UNABLE_TO_READ_DRIVE;
	}
#else
	SET_DPARAM32(0, 0);
	return STR_4005_BYTES_FREE; 
#endif
}

void FiosMakeSavegameName(char *buf, const char *name)
{
	sprintf(buf, "%s/%s.sav", _fios_path, name);
}

void FiosDelete(const char *name)
{
	char *path = str_buffr;
	FiosMakeSavegameName(path, name);
	unlink(path);
}

const DriverDesc _video_driver_descs[] = {
	{"null", "Null Video Driver",				&_null_video_driver,	0},
#if defined(WITH_SDL)
	{"sdl", "SDL Video Driver",					&_sdl_video_driver,	1},
#endif
	{NULL}
};

const DriverDesc _sound_driver_descs[] = {
	{"null", "Null Sound Driver",			&_null_sound_driver,	0},
#if defined(WITH_SDL)
	{"sdl", "SDL Sound Driver",				&_sdl_sound_driver,	1},
#endif
	{NULL}
};

const DriverDesc _music_driver_descs[] = {
#ifndef __BEOS__
	{"extmidi", "External MIDI Driver",	&_extmidi_music_driver,	0},
#endif
	{"null", "Null Music Driver",		&_null_music_driver,	1},
	{NULL}
};

bool FileExists(const char *filename)
{
	return access(filename, 0) == 0;	
}

static int LanguageCompareFunc(const void *a, const void *b)
{
	return strcmp(*(char**)a, *(char**)b);
}

int GetLanguageList(char **languages, int max)
{
	DIR *dir;
	struct dirent *dirent;
	int num = 0;

	dir = opendir(".");
	if (dir != NULL) {
		while ((dirent = readdir(dir))) {
			char *t = strrchr(dirent->d_name, '.');
			if (t && !strcmp(t, ".lng")) {
				languages[num++] = strdup(dirent->d_name);
				if (num == max) break;
			}
		}
		closedir(dir);
	}

	qsort(languages, num, sizeof(char*), LanguageCompareFunc);
	return num;
}

static void ChangeWorkingDirectory(char *exe)
{
	char *s = strrchr(exe, '/');
	if (s != NULL) {
		*s = 0;
		chdir(exe);
		*s = '/';
	}
}

void ShowInfo(const char *str)
{
	puts(str);
}

int CDECL main(int argc, char* argv[])
{
	// change the working directory to enable doubleclicking in UIs
#if defined(__BEOS__) || defined(__APPLE__)
	ChangeWorkingDirectory(argv[0]);
#endif

	_random_seed_2 = _random_seed_1 = time(NULL);


	return ttd_main(argc, argv);
}
