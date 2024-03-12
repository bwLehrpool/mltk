#define NTDDI_VERSION  NTDDI_VISTA
#define WINVER 0x0602
#define _WIN32_WINNT 0x0602
#define WIN32_LEAN_AND_MEAN
#define _UNICODE
#define UNICODE
#define NO_SHLWAPI_STRFCNS
#define ENOTSUP 95
#define EAGAIN 11
#include <windows.h>
#include <winsock2.h>
#include <winnetwk.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <initguid.h>
#include <stdint.h>
#include <stdarg.h>
#include <wchar.h>
#include <time.h>
#include <shlobj.h>
#include <shlguid.h>
#include <strsafe.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <shellapi.h>

DEFINE_GUID(ID_IAudioEndpointVolume, 0x5CDF2C82, 0x841E, 0x4546, 0x97, 0x22, 0x0C, 0xF7, 0x40, 0x78, 0x22, 0x9A);
DEFINE_GUID(ID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6);
DEFINE_GUID(ID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);

#define WM_SOCKDATA (WM_APP+1)

#define BUFLEN (200)

typedef struct {
	const char* path;
	const char* pathIp;
	const char* letter;
	const char* shortcut;
	const char* user;
	const char* pass;
	BOOL  success;
} netdrive_t;

static const ssize_t KEYLEN = 16;
#define DRIVEMAX (100)
#define LOGFILELEN (300)
#define SETTINGS_FILE "B:\\OPENSLX.INI"
#define SETTINGS_FILE_W L"B:\\OPENSLX.INI"

static BOOL _debug = FALSE;

static HINSTANCE hKernel32, hShell32, hUser32;
static OSVERSIONINFO winVer;
static BOOL shareFileOk = FALSE;
static netdrive_t drives[DRIVEMAX];
static wchar_t desktopPath[MAX_PATH+1], tempPath[MAX_PATH+1], programsPath[MAX_PATH+1], windowsPath[MAX_PATH+1];
static wchar_t logFile[LOGFILELEN];
static DWORD _startTime;
#define FS_UNKNOWN (-1)
#define FS_ERROR (0)
#define FS_OK (1)
static int _folderStatus = FS_UNKNOWN; // -1 = Not handled yet, 0 = patching failed, 1 = remapped ok
#define RM_NONE (0)
#define RM_NATIVE (1)
#define RM_NATIVE_FALLBACK (2)
#define RM_VMWARE (3)
static int _remapMode = RM_NONE;
static const char* _remapHomeDrive = NULL;
static BOOL _passCreds = FALSE;
static BOOL _noHomeWarn = FALSE;
static BOOL _deletedCredentials = FALSE;
static BOOL _scriptDone = TRUE, _mountDone = TRUE; // Will be set to false if we actually wait for something...
static char *shost = NULL, *sport = NULL, *suser = NULL, *spass = NULL;

#define SCRIPTFILELEN (50)
static wchar_t _scriptFile[SCRIPTFILELEN];

struct {
	BOOL documents;
	BOOL downloads;
	BOOL desktop;
	BOOL media;
	BOOL other;
} remap;
static BOOL _createMissingRemap = FALSE;

static void setPowerState();
static int setResolution();
static int optimizeForRemote();
static int muteSound(BOOL bMute);
static int setShutdownText();
static void readShareFile();
static BOOL mountNetworkShares(int attemptNo);
static int queryPasswordDaemon();
static BOOL fileExists(wchar_t* szPath);
static BOOL folderExists(wchar_t* szPath);
static wchar_t* escapeShellArg(wchar_t* in, wchar_t *out, wchar_t *end);
static void patchUserPaths(wchar_t *letter);
static void remapViaSharedFolder();

static HRESULT createFolderShortcut(wchar_t* sTargetfile, wchar_t* sLinkfile, wchar_t* comment);

static void alog(const char *fmt, ...)
{
	FILE *f = _wfopen(logFile, L"a+");
	if (f == NULL) return;
	time_t raw = time(NULL);
	struct tm *tinf;
	char buffer[80];
	tinf = localtime(&raw);
	strftime(buffer, 80, "%I:%M:%S ", tinf);
	fputs(buffer, f);
	va_list args;
	va_start(args, fmt);
	vfprintf(f, fmt, args);
	va_end(args);
	fputc('\n', f);
	fclose(f);
}

static void wlog(const wchar_t *fmt, ...)
{
	wchar_t wbuffer[1000];
	char    abuffer[1000];
	va_list args;

	FILE *f = _wfopen(logFile, L"a+");
	if (f == NULL) return;
	time_t raw = time(NULL);
	struct tm *tinf;
	tinf = localtime(&raw);
	strftime(abuffer, 1000, "%I:%M:%S ", tinf);
	fputs(abuffer, f);

	va_start(args, fmt);
	StringCchVPrintfW(wbuffer, 1000, fmt, args);
	va_end(args);
	if (WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, abuffer, 1000, NULL, NULL) == 0) {
		snprintf(abuffer, 1000, "Cannot wlog: widechar to utf8 failed.");
	}
	fputs(abuffer, f);
	fputc('\n', f);
	fclose(f);
}

#define dalog(...) do { if (_debug) alog(__VA_ARGS__); } while (0)
#define dwlog(...) do { if (_debug) wlog(__VA_ARGS__); } while (0)

static void CALLBACK resetShutdown(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	static BOOL bInProc = FALSE;
	if (!bInProc) {
		bInProc = TRUE;
		setShutdownText();
		bInProc = FALSE;
	}
}

static void CALLBACK tmrResolution(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	static BOOL bInProc = FALSE;
	if (!bInProc) {
		bInProc = TRUE;
		if (setResolution() == 0) {
			KillTimer(hWnd, idEvent);
		}
		bInProc = FALSE;
	}
}

static void CALLBACK setupNetworkDrives(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	static BOOL bInProc = FALSE;
	static int attempts = 0;
	if (bInProc || _mountDone)
		return;
	bInProc = TRUE;
	if (!shareFileOk) {
		dalog("No shareFileOk in sND");
		if (_remapMode == RM_NATIVE_FALLBACK || _remapMode == RM_VMWARE) {
			remapViaSharedFolder();
		}
	} else {
		int ret = queryPasswordDaemon();
		dalog("sND: qPD = %d", ret);
		if (ret != 0) {
			if (++attempts < 10)
				goto exit_func;
			alog("queryPasswordDaemon returned %d", ret);
		} else {
			if (!mountNetworkShares(attempts)) {
				if (GetTickCount() - _startTime < 30000 && ++attempts < 15)
					goto exit_func;
			}
		}
		// Finished successfully or failed completely
		if (_folderStatus != FS_OK && (_remapMode == RM_NATIVE_FALLBACK || _remapMode == RM_VMWARE)) {
			remapViaSharedFolder();
		}
	}
	_mountDone = TRUE;
	KillTimer(hWnd, idEvent);
	if (!_noHomeWarn) { // Warn if mapping failed and error is not muted
		if (_folderStatus != FS_OK && shost != NULL && shost[0] == '-' && sport != NULL && sport[0] == '-') {
			MessageBoxA(NULL, "Kein Home-Verzeichnis konfiguriert. Bitte nichts Wichtiges in der VM speichern, sondern z.B. einen USB-Stick verwenden, bzw. evtl. vorhandene Netzlaufwerke verwenden.", "Warnung", MB_ICONERROR);
		} else if (_folderStatus == FS_ERROR) {
			MessageBoxA(NULL, "Fehler beim Einbinden des Home-Verzeichnisses. Bitte nichts Wichtiges in der VM speichern, sondern z.B. einen USB-Stick verwenden.", "Warnung", MB_ICONERROR);
		} else if (_folderStatus == FS_UNKNOWN) {
			MessageBoxA(NULL, "Kein Home-Verzeichnis gefunden. Bitte nichts Wichtiges in der VM speichern, sondern z.B. einen USB-Stick verwenden.", "Warnung", MB_ICONERROR);
		}
	}
	return;
exit_func:
	bInProc = FALSE;
}

static inline int mapScriptVisibility(int input)
{
	if (input == 0)
		return SW_HIDE;
	if (input == 2)
		return SW_SHOWMINNOACTIVE;
	return SW_SHOWNORMAL;
}

static void CALLBACK launchRunscript(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	static int fails = 0;
	wchar_t params[BUFLEN] = L"";
	wchar_t emptyParams[BUFLEN] = L"";
	wchar_t nuser[BUFLEN] = L"";
	wchar_t npass[BUFLEN] = L"";

	if (_scriptDone) {
		KillTimer(hWnd, idEvent);
		return;
	}
	// Prepare credentials cmdline
	if (spass == NULL) {
		dalog("launchRun: No spass");
		goto failure;
	}
	BOOL ok = TRUE;
	ok = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)suser, -1, (LPWSTR)nuser, BUFLEN) > 0 && ok;
	ok = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)spass, -1, (LPWSTR)npass, BUFLEN) > 0 && ok;
	if (!ok) {
		alog("Could not convert user/password to unicode");
		goto failure;
	}
	// Build command line with user password
	wchar_t *end = params + BUFLEN - 2;
	wchar_t *ptr = params;
	ptr = escapeShellArg(nuser, ptr, end);
	*ptr++ = ' ';
	ptr = escapeShellArg(npass, ptr, end);
	*ptr = '\0';
	// Build command line without password, just user
	end = emptyParams + BUFLEN - 2;
	ptr = emptyParams;
	ptr = escapeShellArg(nuser, ptr, end);
	*ptr = '\0';
	if (_debug) {
		wlog(L"Params are '%s'", emptyParams);
	}

	// Scan folder
	WIN32_FIND_DATAA fdFile;
	HANDLE hFind = NULL;
	const char* sDir = "B:\\adminrun";
	char sFile[200];
	snprintf(sFile, 200, "%s\\*-*-*", sDir);
	if ((hFind = FindFirstFileA(sFile, &fdFile)) != INVALID_HANDLE_VALUE) {
		do {
			if(strcmp(fdFile.cFileName, ".") == 0 || strcmp(fdFile.cFileName, "..") == 0)
				continue;
			if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			snprintf(sFile, 200, "%s\\%s", sDir, fdFile.cFileName);
			wchar_t ucPath[BUFLEN];
			ok = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)sFile, -1, (LPWSTR)ucPath, BUFLEN) > 0;
			if (!ok) {
				alog("Cannot convert %s to unicode", sFile);
				continue;
			}
			int index, visibility, passCreds;
			if (sscanf(fdFile.cFileName, "%d-%d-%d", &index, &visibility, &passCreds) < 3) {
				alog("Cannot parse %s", fdFile.cFileName);
				continue;
			}
			ShellExecuteW(NULL, L"open", ucPath, passCreds ? params : emptyParams, L"B:\\", mapScriptVisibility(visibility));
		} while(FindNextFileA(hFind, &fdFile)); //Find the next file.
		FindClose(hFind); //Always, Always, clean things up!
	}

	// Execute legacy runscript
	int scriptVisibility = GetPrivateProfileIntA("openslx", "scriptVisibility", 1, SETTINGS_FILE);
	int nShowCmd = mapScriptVisibility(scriptVisibility); // show window as default

	ShellExecuteW(NULL, L"open", _scriptFile, _passCreds ? params : emptyParams, L"B:\\", nShowCmd);

	// DONE
	_scriptDone = TRUE;
	return;
failure:
	if (++fails > 12) {
		_scriptDone = TRUE;
	}
}

typedef HRESULT (*GFPTYPE)(HWND, int, HANDLE, DWORD, wchar_t*);
typedef HRESULT (*GSFTYPE)(HWND, int, ITEMIDLIST**);
typedef BOOL (*ID2PTYPE)(const ITEMIDLIST*, wchar_t*);

/**
 * Load given path (CSIDL). Store in default (must be allocated to hold at least MAX_PATH+1 chars).
 * If it could not be retrieved by CSIDL and envName is not NULL, it will be read from the
 * environment if possible.
 * fallback will be used if everything else fails.
 * fallback can be NULL, in which case the fallback is empty.
 */
static void loadPath(wchar_t *dest, int csidl, wchar_t *envName, wchar_t *fallback)
{
	if (hShell32 != NULL) {
		GFPTYPE aGetFolderPath = (GFPTYPE)GetProcAddress(hShell32, "SHGetFolderPathW");
		if (aGetFolderPath != NULL) {
			if ((aGetFolderPath)(HWND_DESKTOP, csidl, NULL, SHGFP_TYPE_CURRENT, dest) == S_OK)
				return;
		}
		// Fallback
		GSFTYPE aGetSpecialFolder = (GSFTYPE)GetProcAddress(hShell32, "SHGetSpecialFolderLocation");
		ID2PTYPE aPathToId = (ID2PTYPE)GetProcAddress(hShell32, "SHGetPathFromIDListW");
		if (aGetSpecialFolder != NULL && aPathToId != NULL) {
			ITEMIDLIST *list = NULL;
			HRESULT ret1 = (aGetSpecialFolder)(HWND_DESKTOP, csidl, &list);
			BOOL ret2 = FALSE;
			if (ret1 == 0) {
				ret2 = (aPathToId)(list, dest);
			}
			if (list != NULL) {
				CoTaskMemFree(list);
			}
			if (ret2)
				return;
		}
	}
	if (envName != NULL) {
		// Fallback
		DWORD ret = GetEnvironmentVariableW(envName, dest, MAX_PATH+1);
		if (ret > 0 && ret <= MAX_PATH)
			return;
	}
	if (fallback != NULL) {
		StringCchPrintfW(programsPath, MAX_PATH+1, fallback);
		return;
	}
	*dest = '\0';
}

static void loadPaths()
{
	// Determine a couple of default directories
	// dest, id, env, fallback
	loadPath(programsPath, CSIDL_PROGRAM_FILES, L"ProgramFiles", L"C:\\Program Files");
	loadPath(windowsPath, CSIDL_WINDOWS, L"windir", L"C:\\WINDOWS");
	loadPath(desktopPath, CSIDL_DESKTOPDIRECTORY, NULL, NULL);
	if (GetTempPathW(MAX_PATH+1, tempPath) == 0) {
		DWORD ret = GetEnvironmentVariableW(L"TEMP", tempPath, MAX_PATH+1);
		if (ret == 0 || ret > MAX_PATH) {
			tempPath[0] = 0;
		}
	}
	//wlog(L"Programs: %s, Windows: %s, Desktop: %s, Temp: %s", programsPath, windowsPath, desktopPath, tempPath);
	StringCchPrintfW(logFile, LOGFILELEN, L"%s\\%s", desktopPath, L"openslx.log");
	FILE *tfh = _wfopen(logFile, L"a+");
	if (tfh == NULL) {
		StringCchPrintfW(logFile, LOGFILELEN, L"%s\\%s", tempPath, L"openslx.log");
		tfh = _wfopen(logFile, L"a+");
	}
	if (tfh != NULL) {
		fseek(tfh, 0, SEEK_END);
		long pos = ftell(tfh);
		fclose(tfh);
		if (pos < 3) {
			_wremove(logFile);
		}
	}
	// Read settings from ini file
	remap.documents = GetPrivateProfileIntA("remap", "documents", 1, SETTINGS_FILE) != 0;
	remap.downloads = GetPrivateProfileIntA("remap", "downloads", 1, SETTINGS_FILE) != 0;
	remap.desktop = GetPrivateProfileIntA("remap", "desktop", 0, SETTINGS_FILE) != 0;
	remap.media = GetPrivateProfileIntA("remap", "media", 1, SETTINGS_FILE) != 0;
	remap.other = GetPrivateProfileIntA("remap", "other", 0, SETTINGS_FILE) != 0;
	_createMissingRemap = GetPrivateProfileIntA("openslx", "createMissingRemap", 1, SETTINGS_FILE) != 0;
	_remapMode = GetPrivateProfileIntA("openslx", "remapMode", RM_NATIVE_FALLBACK, SETTINGS_FILE);
	if (_remapMode == RM_NONE) {
		_folderStatus = FS_OK;
	}
	char buffer[100];
	GetPrivateProfileStringA("openslx", "homeDrive", "H:", buffer, sizeof(buffer), SETTINGS_FILE);
	buffer[0] = toupper(buffer[0]);
	buffer[1] = ':';
	buffer[2] = '\0';
	_remapHomeDrive = strdup(buffer);
	// Get extension for autorun script
	int bl = snprintf(buffer, sizeof(buffer), "B:\\runscript");
	GetPrivateProfileStringA("openslx", "scriptExt", "", buffer + bl, sizeof(buffer) - bl, SETTINGS_FILE);
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buffer, -1, (LPWSTR)_scriptFile, SCRIPTFILELEN) <= 0) {
		_scriptFile[0] = '\0';
	}
	// Pass creds to normal runscript?
	_passCreds = GetPrivateProfileIntA("openslx", "passCreds", 0, SETTINGS_FILE) != 0;
	// No warning if no home directory could be mounted
	_noHomeWarn = GetPrivateProfileIntA("openslx", "noHomeWarn", 0, SETTINGS_FILE) != 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	hKernel32 = LoadLibraryW(L"kernel32.dll");
	if (hKernel32 == NULL) {
		alog("Cannot load kernel32.dll");
	}
	hShell32 = LoadLibraryW(L"shell32.dll");
	if (hShell32 == NULL) {
		alog("Cannot load shell32.dll");
	}
	hUser32 = LoadLibraryW(L"user32.dll");
	if (hUser32 == NULL) {
		alog("Cannot load user32.dll");
	}
	winVer.dwOSVersionInfoSize = sizeof(winVer);
	BOOL retVer = GetVersionEx(&winVer);
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	_startTime = GetTickCount();
	loadPaths();
	if (lpCmdLine != NULL && strstr(lpCmdLine, "/debug") != NULL) {
		_debug = TRUE;
	}
	if (!_debug && GetPrivateProfileIntA("openslx", "debug", 0, SETTINGS_FILE) != 0) {
		_debug = TRUE;
	}
	if (_debug) {
		alog("Windows Version %d.%d", (int)winVer.dwMajorVersion, (int)winVer.dwMinorVersion);
	}
	// Mute sound?
	BOOL mute = GetPrivateProfileIntA("openslx", "muteSound", 1, SETTINGS_FILE) != 0;
	if (retVer && winVer.dwMajorVersion >= 6)
		muteSound(mute);
	// Disable screen saver as it might give the false impression that the session is securely locked
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);
	// Same with standby
	setPowerState();
	// Any network shares to mount?
	readShareFile();
	if (shareFileOk || _remapMode != RM_NONE) {
		UINT_PTR tRet = SetTimer(NULL, 0, 1650, (TIMERPROC)&setupNetworkDrives);
		dalog("init: &setupNetworkDrives");
		if (tRet == 0) {
			alog("Could not create timer for mounting network shares: %d", (int)GetLastError());
		} else {
			_mountDone = FALSE;
		}
	}
	// Shutdown button label
	if (retVer && winVer.dwMajorVersion == 6 && winVer.dwMinorVersion == 1) {
		// Only on Windows 7
		// Repeatedly set caption
		UINT_PTR tRet = SetTimer(NULL, 0, 5230, (TIMERPROC)&resetShutdown);
		if (tRet == 0) {
			alog("Could not create timer for shutdown button: %d", (int)GetLastError());
		}
	}
	// Resolution
	UINT_PTR tRet;
	tRet = SetTimer(NULL, 0, 211, (TIMERPROC)&tmrResolution);
	if (tRet == 0) {
		alog("Could not create timer for resolution setting: %d", (int)GetLastError());
	}
	// Runscript
	tRet = SetTimer(NULL, 0, 3456, (TIMERPROC)&launchRunscript);
	dalog("init: &launchRunscript");
	if (tRet == 0) {
		alog("Could not create timer for runscript: %d", (int)GetLastError());
	} else {
		_scriptDone = FALSE;
	}
	// Remote?
	do {
		char buffer[100];
		GetPrivateProfileStringA("openslx", "runMode", "", buffer, sizeof(buffer), SETTINGS_FILE);
		if (strcmp(buffer, "remoteaccess") == 0) {
			optimizeForRemote();
		}
	} while (0);
	// Message pump
	MSG Msg;
	while(GetMessage(&Msg, NULL, 0, 0) > 0) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
		if (!_deletedCredentials && _mountDone && _scriptDone) {
			if (spass != NULL) {
				dalog("Erasing password from memory");
				SecureZeroMemory(spass, strlen(spass));
				_deletedCredentials = TRUE;
			}
		}
	}
	FreeLibrary(hKernel32);
	FreeLibrary(hShell32);
	FreeLibrary(hUser32);
	return 0;
}

static BOOL fileExists(wchar_t* szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

static BOOL folderExists(wchar_t* szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

static wchar_t* escapeShellArg(wchar_t* in, wchar_t *out, wchar_t *end)
{
	int bs;
	end -= 2;
	if (out >= end)
		return out;

	*out++ = '"';
	while (*in != '\0') {
		bs = 0;
		while (*in == '\\' && out < end) {
			bs++;
			*out++ = *in++;
		}
		if (*in == '\0' || *in == '"') {
			while (bs-- > 0 && out < end) {
				*out++ = '\\';
			}
			if (*in == '\0') {
				break;
			}
		}
		if (*in == '"' && out < end) {
			*out++ = '\\';
		}
		if (out < end) {
			*out++ = *in++;
		}
	}
	if (out < end) {
		*out++ = '"';
	}
	*out = '\0';
	return out;
}

static int execute(wchar_t *path, wchar_t *arguments)
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	if (!CreateProcessW(path, arguments, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		return -1;
	}
	while (MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_SENDMESSAGE) == WAIT_OBJECT_0+1) {
		MSG Msg;
		while(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE)) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	DWORD exitCode;
	BOOL ret = GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	if (!ret) {
		return -2;
	}
	return (int)exitCode;
}

typedef EXECUTION_STATE (WINAPI *TETYPE)(EXECUTION_STATE);

static void setPowerState()
{
	// Disable standby and idle-mode (this is a VM!)
	if (hKernel32 == NULL || winVer.dwMajorVersion < 5 || (winVer.dwMajorVersion == 5 && winVer.dwMinorVersion < 1))
		return;
	TETYPE aExecState = (TETYPE)GetProcAddress(hKernel32, "SetThreadExecutionState");
	if (aExecState == NULL) {
		alog("Cannot get SetThreadExecutionState from kernel32.dll");
		return;
	}
	if (winVer.dwMajorVersion >= 6) { // Vista+
		(aExecState)(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED);
	} else { // XP/2003
		(aExecState)(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
	}
}

typedef LONG (WINAPI *CDSTYPE)(LPCWSTR, PDEVMODEW, HWND, DWORD, LPVOID);
typedef BOOL (WINAPI *EDDTYPE)(LPCWSTR, DWORD, PDISPLAY_DEVICEW, DWORD);
typedef BOOL (WINAPI *EDMTYPE)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
typedef BOOL (WINAPI *GMITYPE)(HMONITOR, LPMONITORINFO);

struct resolution {
	long int w, h;
};

#define MAX_SCREENS (16)
static int isResolutionFine(struct resolution *res, int nres);
static int setResWinMulti(struct resolution *res, int nres);
static int setResWinLegacy(struct resolution *res, int nres);
static int setResVMware(struct resolution *res, int nres);

static int setResolution()
{
	static int nres = 0;
	static struct resolution res[MAX_SCREENS];
	static int callCount = -1;
	callCount++;
	if (nres == -1 || callCount > 10) // We've been here before and consider config invalid, or are done
		return 0;
	if (nres == 0) {
		// use config file in floppy
		char data[300] = "";
		GetPrivateProfileStringA("openslx", "resolution2", "", data, sizeof(data), SETTINGS_FILE); // Multi-res, space separated
		if (data[0] == '\0') {
			GetPrivateProfileStringA("openslx", "resolution", "", data, sizeof(data), SETTINGS_FILE); // Fallback
		}
		char *pos = data, *end;
		while (*pos != '\0' && nres < 16) {
			res[nres].w = strtol(pos, &end, 10);
			if (*end != 'x')
				break;
			res[nres].h = strtol(end + 1, &pos, 10);
			if (*pos != '\0' && *pos != ' ')
				break;
			if (res[nres].w < 320 || res[nres].h < 240) {
				alog("Invalid resolution in " SETTINGS_FILE ": '%s' (parsed width=%ld, height=%ld)", data, res[nres].w, res[nres].h);
				continue;
			}
			nres++;
		}
		if (*pos != '\0') {
			alog("Malformed resolution in " SETTINGS_FILE ": '%s'", data);
		}
		if (nres == 0) {
			// Nothing found -- consider this success
			nres = -1;
			dalog("No resolution information in openslx.ini -- doing nothing");
			return 0;
		}
	}
	int check, ret;
	check = isResolutionFine(res, nres);
	if (check == 0)
		return 0; // Yay! Save the hassle.
	switch (callCount % 3) {
		case 0:
			// Use WinAPI first
			ret = setResWinMulti(res, nres);
			if (ret != ENOTSUP)
				break;
			// Fallthrough
		case 1:
			// Try vmware tools
			ret = setResVMware(res, nres);
			if (ret != ENOTSUP)
				break;
		default:
			// Legacy WinAPI (single screen only)
			ret = setResWinLegacy(res, nres);
			if (ret == 0 && nres > 1 && callCount == 2) {
				// Legacy winapi worked, but if we have more than one screen, pretend it failed
				// the first time, so maybe one of the methods above will work if we call them again
				// (Looking at you, VMwareResolutionSet)
				ret = EAGAIN; // Fake failure
			}
			break;
	}
	if (ret == 0 && check == ENOTSUP)
		return 0; // Couldn't query, attempt to set didn't yield an error -> OK
	return EAGAIN; // Otherwise, regardless of attempt's return value, call again,
	// in which case isResolutionFine should yield success.
}

static int isResolutionFine(struct resolution *res, int nres)
{
	EDDTYPE edd = (EDDTYPE)GetProcAddress(hUser32, "EnumDisplayDevicesW");
	if (edd == NULL) {
		// Old Windows with no multiscreen support
		DEVMODEW mode = { .dmSize = sizeof(mode) };
		int query = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &mode);
		if (query == 0) {
			dalog("Could not query current resolution for old Windows.");
			return ENOTSUP;
		}
		return res[0].w == mode.dmPelsWidth && res[0].h == mode.dmPelsHeight;
	}
	// Modern/multi-screen aware approach
	DISPLAY_DEVICEW screen = { .cb = sizeof(screen) };
	int offsets[MAX_SCREENS] = { 0 };
	DWORD screenNum = 0;
	// First make a list of expected offsets for our resolutions
	for (int i = 1; i < nres; ++i) {
		offsets[i] = offsets[i - 1] + res[i - 1].w;
	}
	while (edd(NULL, screenNum++, &screen, 0)) {
		DEVMODEW mode = { .dmSize = sizeof(mode) };
		int query = EnumDisplaySettingsW(screen.DeviceName, ENUM_CURRENT_SETTINGS, &mode);
		if (query == 0 || mode.dmPelsWidth == 0)
			continue; // Not active
		// See if this screen matches any of the expected res+offset
		BOOL found = FALSE;
		for (int i = 0; i < nres; ++i) {
			if (offsets[i] == mode.dmPosition.x
					&& res[i].w == mode.dmPelsWidth && res[i].h == mode.dmPelsHeight) {
				offsets[i] = -1; // Marker
				found = TRUE;
				break;
			}
		}
		if (!found) {
			dalog("Non-matching screen active (%dx%d @ %d)", mode.dmPelsWidth, mode.dmPelsHeight,
					mode.dmPosition.x);
			return EAGAIN;
		}
	}
	// If all offsets are set to -1, everything matched up perfectly
	for (int i = 0; i < nres; ++i) {
		if (offsets[i] != -1) {
			dalog("Expected screen %dx%d @ %d not found in active setup", res[i].w, res[i].h, offsets[i]);
			return EAGAIN;
		}
	}
	dalog("Current screen layout matches expected one");
	return 0;
}

static BOOL foobar(HMONITOR Arg1, HDC Arg2, LPRECT Arg3, LPARAM Arg4)
{
	GMITYPE gmi = (GMITYPE)GetProcAddress(hUser32, "GetMonitorInfoW");
	if (gmi == NULL)
		return FALSE;
	MONITORINFOEXW info = { .cbSize = sizeof(info) };
	if (gmi(Arg1, (LPMONITORINFO)&info) == 0)
		dalog("MonitorInfo FAILED for %d", (int)Arg1);
	else {
		dalog("MonitorInfo for %d:", (int)Arg1);
		dalog("Flags: %d", (int)info.dwFlags);
		dwlog(L"Name: %s", info.szDevice);
	}
	return TRUE;
}

/*
 * This seems to be broken with VirtualBox, and I'm not sure whether
 * I'm doing something wrong here. For a dualscreen setup, this
 * returns two devices, but only the first one has a monitor, i.e.
 * the inner loop never runs for the second device.
 * I'm still leaving this here as a starting-point for future
 * changes or new virtualizers.
 */
static int setResWinMulti(struct resolution *res, int nres)
{
	if (hUser32 == NULL)
		return ENOTSUP;
	CDSTYPE cdsEx = (CDSTYPE)GetProcAddress(hUser32, "ChangeDisplaySettingsExW");
	EDDTYPE edd = (EDDTYPE)GetProcAddress(hUser32, "EnumDisplayDevicesW");
	if (cdsEx == NULL || edd == NULL)
		return ENOTSUP;
	DISPLAY_DEVICEW ddev = { .cb = sizeof(ddev) };
	int ires = 0;
	int chret;
	long int sx = 0;
	BOOL ok = TRUE;
	dalog("WinAPI multiscreen");
	// XXX Debug
	EDMTYPE edm = (EDMTYPE)GetProcAddress(hUser32, "EnumDisplayMonitors");
	if (edm != NULL) {
		edm(NULL, NULL, (MONITORENUMPROC)&foobar, 0);
		dalog("End of monitor enum dump");
	}
	// XXX END DEBUG
	DISPLAY_DEVICEW screen = { .cb = sizeof(screen) };
	DWORD screenNum = 0;
	while (edd(NULL, screenNum++, &screen, 0)) {
		DEVMODEW mode = { .dmSize = sizeof(mode) };
		int query = EnumDisplaySettingsW(screen.DeviceName, ENUM_CURRENT_SETTINGS, &mode);
		if (query == 0) {
			dalog("EnumDisplaySettings: Retrying with index 0");
			query = EnumDisplaySettingsW(screen.DeviceName, 0, &mode);
		}
		mode.dmFields = 0;
		if (query == 0) {
			dalog("EnumDisplaySettings: Falling back to default values");
			mode.dmFields |= DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
			mode.dmBitsPerPel = 32;
			mode.dmDisplayFrequency = 60;
		}
		dwlog(L"%s (%s) currently %dx%d+%d+%d, %dHz, %dbpp (Query: %d)",
				screen.DeviceName, screen.DeviceString,
				mode.dmPelsWidth, mode.dmPelsHeight, mode.dmPosition.x, mode.dmPosition.y,
				mode.dmDisplayFrequency, mode.dmBitsPerPel, query);
		if (ires < nres) {
			// Enable
			mode.dmPelsWidth = res[ires].w;
			mode.dmPelsHeight = res[ires].h;
			mode.dmPosition.x = sx;
			mode.dmPosition.y = 0;
			sx += mode.dmPelsWidth;
			mode.dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;
		} else {
			// Disable
			mode.dmPelsWidth = mode.dmPelsHeight = 0;
			mode.dmFields = DM_POSITION;
		}
		dwlog(L"New: %dx%d+%d+%d", mode.dmPelsWidth, mode.dmPelsHeight, mode.dmPosition.x, mode.dmPosition.y);
		chret = cdsEx(screen.DeviceName, &mode, 0, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
		if (chret != DISP_CHANGE_SUCCESSFUL) {
			dalog("Returned %d", chret);
			ok = FALSE;
		}
		ires++;
		screen = (DISPLAY_DEVICEW){ .cb = sizeof(DISPLAY_DEVICEW) };
	}
	chret = cdsEx(NULL, NULL, NULL, 0, NULL);
	if (chret != DISP_CHANGE_SUCCESSFUL || !ok) {
		dalog("Final multiscreen change display call failed: %d", chret);
		return EAGAIN;
	}
	return ires >= nres ? 0 : EAGAIN; // Did we find enough (virtual) screens?
}

static int setResWinLegacy(struct resolution *res, int nres)
{
	dalog("Using legacy single-screen WinAPI");
	// Legacy single screen
	DEVMODE mode = { .dmSize = sizeof(mode) };
	// MSDN recommends to fill the struct first by querying....
	int query = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);
	// Then set our own desired mode
	mode.dmPelsWidth = res[0].w;
	mode.dmPelsHeight = res[0].h;
	mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
	int ret = ChangeDisplaySettings(&mode, CDS_GLOBAL | CDS_UPDATEREGISTRY);
	if (ret != DISP_CHANGE_SUCCESSFUL) {
		ret = ChangeDisplaySettings(&mode, CDS_GLOBAL);
	}
	if (ret != DISP_CHANGE_SUCCESSFUL) {
		ret = ChangeDisplaySettings(&mode, CDS_UPDATEREGISTRY);
	}
	if (ret != DISP_CHANGE_SUCCESSFUL) {
		ret = ChangeDisplaySettings(&mode, 0);
	}
	if (ret != DISP_CHANGE_SUCCESSFUL) {
		dalog("WinAPI Legacy: CDS: %d (should: 0), EDS: %d (should: !0) - Want: %ld * %ld",
				ret, query, res[0].w, res[0].h);
		return EAGAIN;
	}
	return 0;
}

static int setResVMware(struct resolution *res, int nres)
{
	static wchar_t path[MAX_PATH] = L"";
	if (path[0] == 0) {
		StringCchPrintfW(path, MAX_PATH, L"%s\\VMware\\VMware Tools\\VMwareResolutionSet.exe", programsPath);
		if (!fileExists(path)) {
			// Strip (x86) if not found and try again
			wchar_t *x86 = wcsstr(path, L" (x86)");
			if (x86 != NULL) {
				while ((*x86 = *(x86 + 6)) != 0) ++x86;
			}
		}
		if (!fileExists(path)) {
			char buffer[300];
			WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, 300, NULL, NULL);
			dalog("vmware tools not found, using winapi to set resolution (path: %s)", buffer);
			return ENOTSUP;
		}
	} else if (!fileExists(path)) {
		return ENOTSUP;
	}
	wchar_t cmdline[MAX_PATH];
	wchar_t *pos = cmdline;
	size_t rem = MAX_PATH;
	long int sx = 0, i = 0;
	StringCchPrintfExW(pos, rem, &pos, &rem, 0, L"VMwareResolutionSet.exe 0 %d", nres);
	while (rem > 0 && i < nres) {
		StringCchPrintfExW(pos, rem, &pos, &rem, 0, L" , %ld 0 %ld %ld", sx, res[i].w, res[i].h);
		sx += res[i++].w;
	}
	dwlog(L"vmwareRS cmd: %s", cmdline);
	int ret = execute(path, cmdline);
	if (ret == -1) {
		alog("VmwareRes: CreateProcess failed (%d)", (int)GetLastError());
	} else if (ret == -2) {
		alog("VmwareRes: GetExitCode failed (%d)", (int)GetLastError());
	} else if (ret != 0) {
		dalog("VmwareRes: Exit code: %d", ret);
	} else {
		return 0;
	}
	return EAGAIN;
}

static int optimizeForRemote()
{
	LONG ret;
	HKEY hKey;

	ret = RegOpenKeyExW(HKEY_CURRENT_USER,
			L"Control Panel\\Cursors",
			0, KEY_WOW64_64KEY | KEY_READ | KEY_WRITE, &hKey);
	if (ret != ERROR_SUCCESS) {
		alog("Opening registry for optimizing remote access failed: %ld", (long)ret);
		return 1;
	}
	DWORD val = 0;
	ret = RegSetValueExW(hKey, L"Scheme Source", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
	if (ret != ERROR_SUCCESS) {
		alog("Cannot set Scheme Source to 0: %ld", (long)ret);
	}
	static const char *keys[] = {
		"AppStarting", "Arrow", "Crosshair", "Hand", "Help", "IBeam", "No", "NWPen",
		"SizeAll", "SizeNESW", "SizeNS", "SizeNWSE", "SizeWE", "UpArrow", "Wait",
		NULL
	};
	for (const char **key = keys; *key != NULL; ++key) {
		ret = RegSetValueExA(hKey, *key, 0, REG_SZ, (const BYTE*)"", 1);
		if (ret != ERROR_SUCCESS) {
			alog("Cannot set %s to empty string", key);
		}
	}
	RegCloseKey(hKey);
	static const UINT paramsOff[] = {
		SPI_SETCLIENTAREAANIMATION, SPI_SETMOUSEVANISH, SPI_SETDROPSHADOW, SPI_SETMENUFADE,
		SPI_SETTOOLTIPFADE, SPI_SETTOOLTIPANIMATION, SPI_SETSELECTIONFADE, SPI_SETMENUANIMATION,
		SPI_SETLISTBOXSMOOTHSCROLLING, SPI_SETCURSORSHADOW, SPI_SETCOMBOBOXANIMATION,
		0
	}, paramsOn[] = {
		SPI_SETDISABLEOVERLAPPEDCONTENT,
		0
	};
	for (const UINT *s = paramsOff; *s != 0; ++s) {
		SystemParametersInfo(*s, 0, (PVOID)FALSE, SPIF_UPDATEINIFILE);
	}
	for (const UINT *s = paramsOn; *s != 0; ++s) {
		SystemParametersInfo(*s, 0, (PVOID)TRUE, SPIF_UPDATEINIFILE);
	}
	// Try this although it needs admin perms
	ANIMATIONINFO ai = { .cbSize = sizeof(ai), .iMinAnimate = 0, };
	SystemParametersInfo(SPI_SETANIMATION, 0, &ai, SPIF_UPDATEINIFILE);
	// Apply cursors, broadcast changes to whole system
	SystemParametersInfo(SPI_SETCURSORS, 0, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
	return 0;
}

static int muteSound(BOOL bMute)
{
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	HRESULT hr = CoCreateInstance(&ID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &ID_IMMDeviceEnumerator, (LPVOID *)&deviceEnumerator);
	if (hr != S_OK) {
		alog("CoCreateInstance failed. Cannot mute.");
		return 1;
	}
	//deviceEnumerator->lpVtbl->AddRef(deviceEnumerator);
	IMMDevice *defaultDevice = NULL;
	hr = deviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(deviceEnumerator, eRender, eConsole, &defaultDevice);
	if (hr != S_OK) {
		alog("GetDefaultAudioEndpoint failed. Cannot mute.");
		return 2;
	}
	//defaultDevice->lpVtbl->AddRef(defaultDevice);
	//deviceEnumerator->lpVtbl->Release(deviceEnumerator);
	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->lpVtbl->Activate(defaultDevice, &ID_IAudioEndpointVolume, CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	if (hr != S_OK) {
		alog("IMMDevice::Activate() failed. Cannot mute.");
		return 3;
	}
	//endpointVolume->lpVtbl->AddRef(endpointVolume);
	//defaultDevice->lpVtbl->Release(defaultDevice);
	float targetVolume = 1;
	endpointVolume->lpVtbl->SetMasterVolumeLevelScalar(endpointVolume, targetVolume, NULL);
	endpointVolume->lpVtbl->SetMute(endpointVolume, bMute, NULL);
	//endpointVolume->lpVtbl->Release(endpointVolume);
	//CoUninitialize();
	return 0;
}

static int setShutdownText()
{
	HWND hMenu = FindWindowA("DV2ControlHost", NULL);
	if (hMenu == NULL) return 1; // TODO: Enum all of them
	HWND hPane = FindWindowExA(hMenu, NULL, "DesktopLogoffPane", NULL);
	if (hMenu == NULL) return 2;
	HWND hButton = FindWindowExA(hPane, NULL, "Button", NULL);
	if (hButton == NULL) return 3;
	if (SendMessageA(hButton, WM_SETTEXT, 0, (LPARAM)"Abmelden") != TRUE) return 4;
	return 0;
}

static uint8_t *bkey1 = NULL, *bkey2 = NULL;

static char* xorString(const uint8_t* text, int len, const uint8_t* key);
static int getbin(int x);
static uint8_t* hex2bin(char *szHexString);

static char* getToken(char **ptr, BOOL doDup)
{
	if (*ptr == NULL || **ptr == '\0') return NULL;
	char *dest = *ptr;
	while (**ptr != '\0') {
		if (**ptr == '\n' || **ptr == '\r' || **ptr == '\t') {
			*(*ptr)++ = '\0';
			break;
		}
		(*ptr)++;
	}
	if (doDup) {
		dest = strdup(dest);
	}
	return dest;
}

const char* uncReplaceFqdnByIp(const char* path)
{
	if (path == NULL || path[0] != '\\' || path[1] != '\\')
		return NULL;
	const char *host = path + 2;
	const char *rest = strchr(host, '\\');
	if (rest == NULL || rest - host >= 200)
		return NULL;
	char name[201];
	strncpy(name, host, rest - host);
	name[rest-host] = '\0';
	dalog("Trying to resolve '%s'...", name);
	struct hostent *remote = gethostbyname(name);
	if (remote == NULL || remote->h_addrtype != AF_INET || remote->h_addr_list[0] == 0)
		return NULL;
	struct in_addr addr;
	addr.s_addr = *(u_long*)remote->h_addr_list[0];
	char *ip = inet_ntoa(addr);
	if (ip == NULL)
		return NULL;
	size_t len = 2 /* \\ */ + strlen(ip) /* 1.2.3.4 */ + strlen(rest) /* \path\to\share */ + 1 /* nullchar */;
	char *retval = malloc(len);
	snprintf(retval, len, "\\\\%s%s", ip, rest);
	dalog("Turned '%s' into '%s'", path, retval);
	return retval;
}

#define FREENULL(x) do { free((void*)(x)); (x) = NULL; } while (0)

static void readShareFile()
{
	if (shareFileOk)
		return;
	memset(drives, 0, sizeof(drives));
	FILE *h = fopen("B:\\shares.dat", "r");
	if (h == NULL) return;
	char creds[300] = "", buffer[500] = "";
	char *skey1 = NULL, *skey2 = NULL;
	if (fgets(creds, sizeof(creds), h) != NULL) {
		char *ptr = creds;
		shost = getToken(&ptr, TRUE);
		sport = getToken(&ptr, TRUE);
		skey1 = getToken(&ptr, FALSE);
		skey2 = getToken(&ptr, FALSE);
		suser = getToken(&ptr, TRUE);
	}
	int idx = 0;
	while (fgets(buffer, sizeof(buffer), h) != NULL && idx < DRIVEMAX) {
		char *ptr = buffer;
		netdrive_t *d = &drives[idx];
		d->path = getToken(&ptr, TRUE);
		d->letter = getToken(&ptr, TRUE);
		d->shortcut = getToken(&ptr, TRUE);
		d->user = getToken(&ptr, TRUE);
		d->pass = getToken(&ptr, TRUE);
		if (d->path == NULL || d->path[0] == '\0')
			goto drive_fail;
		d->success = FALSE;
		// Hack: For unknown reasons, with some servers mounting fails using a fqdn, but using the ip address instead succeeds.
		d->pathIp = uncReplaceFqdnByIp(d->path);
		//
		idx++;
		continue;
drive_fail:
		FREENULL(d->path);
		FREENULL(d->letter);
		FREENULL(d->shortcut);
		FREENULL(d->user);
		FREENULL(d->pass);
	}
	fclose(h);
	if (shost == NULL || sport == NULL || skey1 == NULL || skey2 == NULL || suser == NULL) {
		// Credential stuff missing
		dalog("Incomplete first line in shares.dat");
		return;
	}
	if (*shost == '-' && *sport == '-') {
		dalog("Demo mode detected");
		shareFileOk = TRUE;
		spass = malloc(1);
		*spass = '\0';
		return;
	}
	if (strlen(skey1) != KEYLEN*2 || strlen(skey2) != KEYLEN*2) // Messed up keys
		return;
	if (atoi(sport) < 1000 || atoi(sport) > 65535) // Invalid port
		return;
	bkey1 = hex2bin(skey1);
	bkey2 = hex2bin(skey2);
	if (bkey1 == NULL || bkey2 == NULL)
		return;
	shareFileOk = TRUE;
}

static void udpReceived(SOCKET sock);

LRESULT CALLBACK slxWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_SOCKDATA) {
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	// Socket event
	int event = LOWORD(lParam);
	int errorCode = HIWORD(lParam);
	if (errorCode == 0 && event == FD_READ) {
		udpReceived((SOCKET)wParam);
	}
	return 0;
}

static int queryPasswordDaemon()
{
	// See if preconditions are met
	dalog("in qPD");
	if (!shareFileOk || spass != NULL)
		return 0;
	dalog("...");
	static int wsaInit = 1337;
	static SOCKET sock = INVALID_SOCKET;
	static HWND sockWnd = NULL;
	// Init socket stuff
	if (wsaInit == 1337) {
		WSADATA wsa;
		wsaInit = WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	if (wsaInit != 0)
		return 2;
	// Create window for socket events
	if (sockWnd == NULL) {
		sockWnd = CreateWindowA("STATIC", "OpenSLX mystery window", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
		if (sockWnd == NULL)
			return GetLastError();
		SetWindowLong(sockWnd, GWL_WNDPROC, (LONG)&slxWindowProc);
	}
	// Create socket
	if (sock == INVALID_SOCKET) {
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == INVALID_SOCKET)
			return 3;
		if (WSAAsyncSelect(sock, sockWnd, WM_SOCKDATA, FD_READ) != 0) {
			alog("WSAAsyncSelect returned %d", (int)WSAGetLastError());
		}
	}
	SOCKADDR_IN remote;
	remote.sin_family = AF_INET;
	remote.sin_port = htons((u_short)atoi(sport));
	remote.sin_addr.s_addr = inet_addr(shost);
	// Send out request for password
	if (sendto(sock, (const char*)bkey1, KEYLEN, 0, (struct sockaddr*)&remote, sizeof(remote)) != KEYLEN)
		return 4;
	if (spass == NULL)
		return -1;
	return 0;
}

static void udpReceived(SOCKET sock)
{
	dalog("UDP received");
	int ret;
	uint8_t buffer[200];
	ret = recv(sock, (char*)buffer, sizeof(buffer), 0);
	// See if reply is valid
	if (ret < 2) return;
	uint16_t len = (uint16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);
	if (ret - 2 != len) return;
	// Success
	spass = xorString(buffer + 2, len, bkey2);
	closesocket(sock);
	mountNetworkShares(0);
}

static DWORD mount(LPNETRESOURCEW share, LPWSTR pass, LPWSTR user)
{
	DWORD retval;
	// Now try to mount
	if ((pass && *pass) || (user && *user)) {
		retval = WNetAddConnection2W(share, pass, user, CONNECT_TEMPORARY | CONNECT_CURRENT_MEDIA);
		if (retval == NO_ERROR) {
			return retval;
		}
		if (retval != ERROR_INVALID_PASSWORD && retval != ERROR_LOGON_FAILURE
				&& retval != ERROR_BAD_USERNAME && retval != ERROR_ACCESS_DENIED
				&& retval != ERROR_SESSION_CREDENTIAL_CONFLICT && retval != ERROR_BAD_NET_NAME
				&& retval != ERROR_NOT_AUTHENTICATED && retval != ERROR_NOT_LOGGED_ON) {
			return retval;
		}
	}
	static wchar_t nuser[BUFLEN] = L"\0", npass[BUFLEN] = L"\0";
	if (nuser[0] == 0 && npass[0] == 0) {
		BOOL ok = TRUE;
		if (suser != NULL) {
			ok = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)suser, -1, (LPWSTR)nuser, BUFLEN) > 0 && ok;
		}
		if (spass != NULL) {
			ok = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)spass, -1, (LPWSTR)npass, BUFLEN) > 0 && ok;
		}
		if (!ok)
			return ERROR_INVALID_PARAMETER;
	}
	retval = WNetAddConnection2W(share, npass, nuser, CONNECT_TEMPORARY | CONNECT_CURRENT_MEDIA);
	return retval;
}

static void postSuccessfulMount(const netdrive_t *d, wchar_t *letter)
{
	wchar_t wShortcut[MAX_PATH] = L"", wUncPath[MAX_PATH];
	BOOL isPrinter = d->letter != NULL && strcmp(d->letter, "PRINTER") == 0;
	MultiByteToWideChar(CP_UTF8, 0, d->path, -1, wUncPath, MAX_PATH);
	if (d->shortcut != NULL && strlen(d->shortcut) != 0) {
		MultiByteToWideChar(CP_UTF8, 0, d->shortcut, -1, wShortcut, MAX_PATH);
	}
	if (wShortcut[0] != '\0' && !isPrinter) {
		wchar_t wLinkName[MAX_PATH], wTarget[MAX_PATH];
		StringCchPrintfW(wLinkName, MAX_PATH, L"%s\\%s.lnk", desktopPath, wShortcut);
		StringCchPrintfW(wTarget, MAX_PATH, L"\"%s\"", wUncPath); // Quote
		DeleteFileW(wLinkName);
		if (letter == NULL || *letter == '\0') {
			createFolderShortcut(wTarget, wLinkName, letter);
		} else if (strstr(d->path, "@SSL") != NULL || strstr(d->path, "webdav") != NULL
				|| strstr(d->path, "WebDav") != NULL || strstr(d->path, "WebDAV") != NULL
				|| strstr(d->path, "@ssl") != NULL || strncmp(d->path, "http", 4) == 0) {
			createFolderShortcut(letter, wLinkName, letter);
		} else {
			createFolderShortcut(wTarget, wLinkName, letter);
		}
		// Fix paths and kill explorer if it's the home directory
		if (_folderStatus != FS_OK && strncmp(d->shortcut, "Home-", 5) == 0) {
			BOOL isVmware = strcmp(d->path, "\\\\vmware-host\\Shared Folders\\home") == 0;
			if (_remapMode == RM_NATIVE_FALLBACK
					|| (isVmware && _remapMode == RM_VMWARE)
					|| (!isVmware && _remapMode == RM_NATIVE)) {
				patchUserPaths(letter);
			}
		}
	}
	if (isPrinter) {
		wchar_t cmdline[MAX_PATH];
		BOOL def = FALSE;
		if (wShortcut[0] != '\0') {
			def = d->shortcut[0] == '@';
			//int offs = (def ? 1 : 0); TODO
		}
		StringCchPrintfW(cmdline, MAX_PATH, L"printui.dll PrintUIEntry /q /in /n \"%s\"", wUncPath);
		SHELLEXECUTEINFOW e = {0}, f = {0};
		e.cbSize = sizeof(e);
		e.lpFile = L"rundll32";
		e.lpParameters = cmdline;
		e.nShow = SW_HIDE;
		f = e; // Copy struct before we set the following flag for the first one
		e.fMask = SEE_MASK_NOCLOSEPROCESS; // So we can wait for the first one
		ShellExecuteExW(&e);
		if (def) { // Only actually do so if we want to set it as default
			WaitForSingleObject(e.hProcess, INFINITE);
		}
		CloseHandle(e.hProcess);
		if (def) {
			StringCchPrintfW(cmdline, MAX_PATH, L"printui.dll PrintUIEntry /q /y /n \"%s\"", wUncPath);
			ShellExecuteExW(&f);
		}
	}
}

static BOOL mountNetworkShare(const netdrive_t *d, BOOL useIp)
{
	wchar_t path[BUFLEN] = L"", user[BUFLEN] = L"", pass[BUFLEN] = L"", letter[10] = L"", shortcut[BUFLEN] = L"";
	int ok = -1;
	if (useIp && (d->pathIp == NULL || d->pathIp[0] == '\0'))
		return FALSE;
	const char *uncPath = useIp ? d->pathIp : d->path;
	if (uncPath == NULL)
		return FALSE;
	ok &= MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)uncPath, -1, (LPWSTR)path, BUFLEN) > 0;
	if (d->letter != NULL) {
		ok &= MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->letter, -1, (LPWSTR)letter, 10) > 0;
	}
	if (d->user != NULL) {
		ok &= MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->user, -1, (LPWSTR)user, BUFLEN) > 0;
	}
	if (d->pass != NULL) {
		ok &= MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->pass, -1, (LPWSTR)pass, BUFLEN) > 0;
	}
	if (d->shortcut != NULL) {
		ok &= MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->shortcut, -1, (LPWSTR)shortcut, BUFLEN) > 0;
	}
	if (!ok || path[0] == 0) { // Convert failed/no path - return true anyways since retrying wouldn't change anything
		alog("mountNetworkShare: utf8 to utf16 failed, or path empty (src: '%s')", uncPath);
		return TRUE;
	}
	DWORD retval;
	NETRESOURCEW share = { 0 };
	share.dwType = RESOURCETYPE_DISK;
	share.lpLocalName = letter;
	share.lpRemoteName = path;
	share.lpProvider = NULL;
	if (letter[0] != 0 && letter[0] != '?') { // ? will pick automatically
		// Try with specific letter
		if (wcscmp(L"PRINTER", letter) == 0) {
			// Printer
			letter[0] = 0;
			share.dwType = RESOURCETYPE_PRINT;
			dalog("Is a printer");
		} else if (letter[0] == '-') {
			// No letter, just use as resource
			letter[0] = 0;
			dalog("Is a resource");
		} else {
			letter[1] = ':';
			letter[2] = 0;
			dalog("Is a share with letter");
		}
		// Connect defined share
		retval = mount(&share, pass, user);
		if (retval == NO_ERROR
				|| (share.dwType == RESOURCETYPE_PRINT && retval == ERROR_SESSION_CREDENTIAL_CONFLICT)) {
			postSuccessfulMount(d, letter);
			return TRUE;
		}
		if (retval != ERROR_ALREADY_ASSIGNED && retval != ERROR_DEVICE_ALREADY_REMEMBERED
				&& retval != ERROR_CONNECTION_UNAVAIL) {
			alog("mountNetworkShare: '%s' with letter failed: %d", uncPath, (int)retval);
			return FALSE;
		}
	}
	if (share.dwType == RESOURCETYPE_DISK && letter[0] != '-') {
		// Try to find free drive letter
		letter[1] = ':';
		letter[2] = 0;
		for (letter[0] = 'Z'; letter[0] > 'C'; --letter[0]) {
			retval = mount(&share, pass, user);
			if (retval == ERROR_ALREADY_ASSIGNED || retval == ERROR_DEVICE_ALREADY_REMEMBERED
					|| retval == ERROR_CONNECTION_UNAVAIL)
				continue;
			if (retval == NO_ERROR) {
				postSuccessfulMount(d, letter);
				return TRUE;
			}
			alog("mountNetworkShare: '%s' without letter failed: %d", uncPath, (int)retval);
			if (retval == ERROR_INVALID_PASSWORD || retval == ERROR_LOGON_FAILURE
					|| retval == ERROR_BAD_USERNAME || retval == ERROR_ACCESS_DENIED
					|| retval == ERROR_SESSION_CREDENTIAL_CONFLICT) {
				return TRUE;
			}
			return FALSE;
		}
	}
	return FALSE;
}

static BOOL mountNetworkShares(int attemptNo)
{
	BOOL withPrinters = attemptNo > 4;
	int failCount = 0;

	if (!shareFileOk)
		return TRUE;
	if (spass == NULL)
		return FALSE;

	for (int i = 0; i < DRIVEMAX; ++i) {
		if (drives[i].path == NULL)
			break;
		if (drives[i].success)
			continue;
		if (!withPrinters && drives[i].letter != NULL
				&& strcmp(drives[i].letter, "PRINTER") == 0) {
			// Delay connecting printers a bit, might need a resource share or other setup
			failCount++;
		} else if (mountNetworkShare(&drives[i], FALSE)) {
			drives[i].success = TRUE;
		} else if (mountNetworkShare(&drives[i], TRUE)) {
			drives[i].success = TRUE;
		} else {
			failCount++;
		}
	}

	return failCount == 0;
}

static void remapViaSharedFolder()
{
	static const char* homeDirA = "\\\\vmware-host\\Shared Folders\\home"; // thiscase!
	static const wchar_t* homeDirW = L"\\\\vmware-host\\shared folders\\home"; // lowercase!
	static BOOL once = FALSE;
	if (once) return;
	once = TRUE;
	netdrive_t d;
	d.path = homeDirA;
	d.letter = _remapHomeDrive;
	d.shortcut = "Home-Verzeichnis";
	d.user = "";
	d.pass = "";
	d.success = FALSE;
	// See if it's already mapped
	wchar_t letter[5] = L"C:\\";
	char buffer[600];
	UNIVERSAL_NAME_INFOW *uni = (UNIVERSAL_NAME_INFOW*)buffer;
	for (letter[0] = 'D'; letter[0] <= 'Z'; ++letter[0]) {
		//wlog(L"Checking %s", letter);
		DWORD len = (DWORD)sizeof(buffer);
		if (NO_ERROR == WNetGetUniversalNameW(letter, UNIVERSAL_NAME_INFO_LEVEL, uni, &len)) {
			_wcslwr(uni->lpUniversalName);
			//wlog(L"Is %s", uni->lpUniversalName);
			if (wcscmp(uni->lpUniversalName, homeDirW) == 0) {
				letter[2] = '\0';
				postSuccessfulMount(&d, letter);
				return;
			}
		}
	}
	// Map vmware shared folder
	mountNetworkShare(&d, FALSE);
}

static char* xorString(const uint8_t* text, int len, const uint8_t* key)
{
	int i;
	uint8_t *retval = malloc(len + 1);
	uint8_t *ptr = retval;
	for (i = 0; i < len; ++i) {
		ptr[i] = text[i] ^ key[i % KEYLEN];
	}
	ptr[len] = '\0';
	return (char*)retval;
}

static int getbin(int x)
{
	if (x >= '0' && x <= '9')
		return x - '0';
	if (x >= 'A' && x <= 'F')
		return x - 'A' + 10;
	return x - 'a' + 10;
}

static uint8_t* hex2bin(char *szHexString)
{
	int size = strlen(szHexString) / 2, i;
	char *p = szHexString;
	uint8_t *pBinary = malloc(size);

	for(i = 0; i < size; i++, p += 2) {
		pBinary[i] = (uint8_t)((getbin(p[0]) << 4) | getbin(p[1]));
	}
	return pBinary;
}

// Stuff for creating a simple shortcut (.lnk)

static HRESULT createFolderShortcut(wchar_t* targetDir, wchar_t* linkFile, wchar_t* comment)
{
	HRESULT       hRes;                  /* Returned COM result code */
	IShellLink*   pShellLink;            /* IShellLink object pointer */
	IPersistFile* pPersistFile;          /* IPersistFile object pointer */

	hRes = E_INVALIDARG;
	if (
		(targetDir != NULL) && (wcslen(targetDir) > 0) &&
		(linkFile != NULL) && (wcslen(linkFile) > 0)
	) {
		hRes = CoCreateInstance(
					&CLSID_ShellLink,     /* pre-defined CLSID of the IShellLink object */
					NULL,                 /* pointer to parent interface if part of aggregate */
					CLSCTX_INPROC_SERVER, /* caller and called code are in same process */
					&IID_IShellLink,      /* pre-defined interface of the IShellLink object */
					(void**)&pShellLink); /* Returns a pointer to the IShellLink object */
		if (SUCCEEDED(hRes)) {
			wchar_t explorer[MAX_PATH];
			StringCchPrintfW(explorer, MAX_PATH, L"\"%s\\explorer.exe\"", windowsPath);
			// Set the fields in the IShellLink object
			hRes = pShellLink->lpVtbl->SetPath(pShellLink, explorer);
			hRes = pShellLink->lpVtbl->SetArguments(pShellLink, targetDir);
			if (comment != NULL) {
				hRes = pShellLink->lpVtbl->SetDescription(pShellLink, comment);
			}
			if (winVer.dwMajorVersion >= 6) { // Vista+
				StringCchPrintfW(explorer, MAX_PATH, L"%s\\system32\\imageres.dll", windowsPath);
				hRes = pShellLink->lpVtbl->SetIconLocation(pShellLink, explorer, 137);
			} else {
				StringCchPrintfW(explorer, MAX_PATH, L"%s\\system32\\shell32.dll", windowsPath);
				hRes = pShellLink->lpVtbl->SetIconLocation(pShellLink, explorer, 85);
			}

			/* Use the IPersistFile object to save the shell link */
			hRes = pShellLink->lpVtbl->QueryInterface(
					pShellLink,                /* existing IShellLink object */
					&IID_IPersistFile,         /* pre-defined interface of the IPersistFile object */
					(void**)&pPersistFile);    /* returns a pointer to the IPersistFile object */
			if (SUCCEEDED(hRes)) {
				hRes = pPersistFile->lpVtbl->Save(pPersistFile, linkFile, TRUE);
				pPersistFile->lpVtbl->Release(pPersistFile);
			}
			pShellLink->lpVtbl->Release(pShellLink);
		}

	}
	return (hRes);
}

// Patch user directories

static BOOL patchRegPath(BOOL *patchOk, BOOL *anyMapped, HKEY hKey, wchar_t *letter, wchar_t *value, ...)
{
	wchar_t *folder = NULL;
	wchar_t first[MAX_PATH] = {0};
	wchar_t path[MAX_PATH];
	wchar_t oldvalue[MAX_PATH];
	va_list args;
	LONG ret;
	DWORD type;
	DWORD len;
	// Let's check the path in the registry first
	len = (DWORD)(sizeof(oldvalue) - sizeof(wchar_t));
	ret = RegQueryValueExW(hKey, value, NULL, &type, (BYTE*)oldvalue, &len);
	if (ret == ERROR_SUCCESS && (type == REG_EXPAND_SZ || type == REG_SZ)) {
		len /= 2;
		oldvalue[len] = '\0';
		if (towlower(oldvalue[0]) == towlower(letter[0]) && folderExists(oldvalue)) // Same drive, folder exists, yay
			return TRUE;
	}
	// Old registry value doesn't fit - figure out new value
	va_start(args, value);
	while ((folder = va_arg(args, wchar_t*)) != NULL) {
		StringCchPrintfW(path, MAX_PATH, L"%s\\%s", letter, folder);
		if (folderExists(path))
			break;
		if (*first == 0) {
			wcsncpy(first, path, MAX_PATH);
		}
	}
	va_end(args);
	if (folder != NULL) {
		// Found something existing
		folder = path;
	} else if (!_createMissingRemap) {
		// Nothing found, must not create
		wlog(L"Cannot remap %s to %s: target not found!", value, first);
		return FALSE;
	} else {
		// Nothing found, use first element of list and create it
		folder = first;
		CreateDirectoryW(folder, NULL);
	}
	_wcslwr(folder);
	_wcslwr(oldvalue);
	if (wcscmp(folder, oldvalue) == 0) {
		// Path already in registry, don't update
		return TRUE;
	}
	ret = RegSetValueExW(hKey, value, 0, REG_SZ, (BYTE*)folder, (wcslen(folder) + 1) * sizeof(wchar_t));
	if (ret == ERROR_SUCCESS) {
		*anyMapped = TRUE;
		return TRUE;
	}
	wlog(L"Setting reg key %s to %s failed (return value %ld)", value, folder, (long)ret);
	*patchOk = FALSE;
	return FALSE;
}

typedef HANDLE (WINAPI *SNTYPE)(DWORD, DWORD);
typedef BOOL (WINAPI *P32TYPE)(HANDLE, PROCESSENTRY32W*);

static void patchUserPaths(wchar_t *letter)
{
	LONG ret;
	HKEY hKey;
	BOOL patchOk = TRUE;
	BOOL killOk = FALSE;
	BOOL anyMapped = FALSE;
	_folderStatus = FS_ERROR;
	ret = RegOpenKeyExW(HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
			0, KEY_WOW64_64KEY | KEY_READ | KEY_WRITE, &hKey);
	if (ret != ERROR_SUCCESS) {
		alog("Opening registry for patching of pathes failed with return code %ld", (long)ret);
		return;
	}
	// Ha!
	const BOOL win10 = winVer.dwMajorVersion >= 10;
	if (remap.other) {
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{56784854-C6CB-462B-8169-88E350ACB882}", L"Contacts", L"Profile\\Contacts", L"Kontakte", NULL);
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"Favorites", L"Favorites", L"Profile\\Favorites", L"Favoriten", NULL);
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{7D1D3A04-DEBB-4115-95CF-2F29DA2920DA}", L"Searches", L"Profile\\Searches", NULL);
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{BFB9D5E0-C6A9-404C-B2B2-AE6DB6AF4968}", L"Links", L"Profile\\Links", NULL);
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4}", L"Saved Games", L"SavedGames", L"Profile\\SavedGames", NULL);
	}
	if (remap.media) {
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"My Video", L"Videos", L"My Videos", L"Eigene Videos", NULL);
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"My Pictures", L"Pictures", L"My Pictures", L"Eigene Bilder", L"Bilder", NULL);
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"My Music", L"Music", L"My Music", L"Eigene Musik", L"Musik", NULL);
		if (win10) {
			patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{35286a68-3c57-41a1-bbb1-0eae73d76c95}", L"Videos", L"My Videos", L"Eigene Videos", NULL);
			patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{0ddd015d-b06c-45d5-8c4c-f59713854639}", L"Pictures", L"My Pictures", L"Eigene Bilder", L"Bilder", NULL);
			patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{a0c69a99-21c8-4671-8703-7934162fcf1d}", L"Music", L"My Music", L"Eigene Musik", L"Musik", NULL);
		}
	}
	if (remap.downloads) {
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{374DE290-123F-4565-9164-39C4925E467B}", L"Downloads", L"Profile\\Downloads", NULL);
		if (win10) {
			patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{7d83ee9b-2244-4e70-b1f5-5393042af1e4}", L"Downloads", L"Profile\\Downloads", NULL);
		}
	}
	if (remap.documents) {
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"Personal", L"Documents", L"Dokumente", L"My Documents", L"Eigene Dateien", NULL);
		if (win10) {
			patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{f42ee2d3-909f-4907-8871-4c22fc0bf756}", L"Documents", L"Dokumente", L"My Documents", L"Eigene Dateien", NULL);
		}
	}
	if (remap.desktop) {
		patchRegPath(&patchOk, &anyMapped, hKey, letter, L"Desktop", L"Windows Desktop", L"Desktop", L"Arbeitsfl\u00E4che", NULL);
		if (win10) {
			patchRegPath(&patchOk, &anyMapped, hKey, letter, L"{B4BFCC3A-DB2C-424C-B029-7FE99A87C641}", L"Windows Desktop", L"Desktop", L"Arbeitsfl\u00E4che", NULL);
		}
	}
	RegCloseKey(hKey);
	if (!anyMapped) {
		_folderStatus = FS_OK;
		return;
	}
	// Kill explorer
	// Late binding
	if (hKernel32 == NULL || winVer.dwMajorVersion < 5 || (winVer.dwMajorVersion == 5 && winVer.dwMinorVersion < 1)) {
		return;
	}
	SNTYPE aCreateSnapshot;
	P32TYPE aProcessFirst, aProcessNext;
	aCreateSnapshot = (SNTYPE)GetProcAddress(hKernel32, "CreateToolhelp32Snapshot");
	if (aCreateSnapshot == NULL) {
		alog("No CreateToolhelp32Snapshot in kernel.dll");
		return;
	}
	aProcessFirst = (P32TYPE)GetProcAddress(hKernel32, "Process32FirstW");
	if (aProcessFirst == NULL) {
		alog("No Process32FirstW in kernel.dll");
		return;
	}
	aProcessNext = (P32TYPE)GetProcAddress(hKernel32, "Process32NextW");
	if (aProcessNext == NULL) {
		alog("No Process32NextW in kernel.dll");
		return;
	}
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE snapshot = (aCreateSnapshot)(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE && (aProcessFirst)(snapshot, &entry)) {
		do {
			if (_wcsicmp(entry.szExeFile, L"explorer.exe") == 0) {
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				if (hProcess == NULL) {
					alog("Cannot OpenProcess explorer.exe: Remapped paths will not work properly (GetLastError=%d)", (int)GetLastError());
				} else {
					if (TerminateProcess(hProcess, 23)) {
						killOk = TRUE;
					}
					CloseHandle(hProcess);
				}
			}
		} while ((aProcessNext)(snapshot, &entry));
	} else {
		alog("Could not get process list");
	}
	CloseHandle(snapshot);
	if (patchOk && killOk) {
		_folderStatus = FS_OK;
	}
}

