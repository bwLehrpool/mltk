#define NTDDI_VERSION  NTDDI_VISTA
#define WINVER 0x0602
#define _WIN32_WINNT 0x0602
//#include <winsock2.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <initguid.h>

DEFINE_GUID(ID_IAudioEndpointVolume, 0x5CDF2C82, 0x841E, 0x4546, 0x97, 0x22, 0x0C, 0xF7, 0x40, 0x78, 0x22, 0x9A);
DEFINE_GUID(ID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6);
DEFINE_GUID(ID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);

static int setResolution();
static int muteSound();
static int setShutdownText();

static void CALLBACK resetShutdown(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	setShutdownText();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	int ret;
	/*
	// Part 1: Auf UDP Paket vom Host warten, mit Anweisungen zur Auflösung
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
	if (ret != 0) return 1;
	SOCKET s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) return 2;
	SOCKADDR_IN local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(2013);
	local.sin_addr.s_addr = ADDR_ANY;
	ret = bind(s, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
	if (ret == SOCKET_ERROR) return 3;
	for (;;) {
		recvfrom(s, 
	}
	*/
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(version);
	BOOL retVer = GetVersionEx(&version);
	// Set resolution to what HOSTRES.TXT says
	setResolution();
	// Mute sound by default
	if (retVer && version.dwMajorVersion >= 6)
		muteSound();
	// Disable screen saver as it might give the false impression that the session is securely locked
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);
	// Disable standby and idle-mode (this is a VM!)
	if (version.dwMajorVersion >= 6) { // Vista+
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED);
	} else { // XP/2003
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
	}
	// Shutdown button label
	if (retVer && version.dwMajorVersion == 6 && version.dwMinorVersion == 1) {
		// Only on Windows 7
		char buffer[100];
		// Repeatedly set caption
		UINT_PTR tRet = SetTimer(NULL, 0, 5000, (TIMERPROC)&resetShutdown);
		if (tRet == 0) {
			snprintf(buffer, 100, "Could not create timer: %d", (int)tRet);
			MessageBoxA(0, buffer, "SetTimer", 0);
			return 0;
		}
		// Message pump
		MSG Msg;
		while(GetMessage(&Msg, NULL, 0, 0) > 0) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	return 0;
}

static int setResolution()
{
	int ret, i;
	int width, height;
	// Quicker way: use config file in floppy
	FILE *h = fopen("B:\\hostres.txt", "rb");
	if (h == NULL) return 4;
	char data[200] = "";
	fread(data, 200, 1, h);
	char *x = strchr(data, 'x');
	if (x == NULL) return 5;
	*x++ = '\0';
	width = atoi(data);
	height = atoi(x);
	fclose(h);
	// Part 2: Auflösung setzen und verabschieden
	DEVMODE mode;
	int query = 1337;
	for (i = 0; i < 6; ++i) {
		memset(&mode, 0, sizeof(mode));
		mode.dmSize = sizeof(mode);
		// MSDN recommends to fill the struct first by querying....
		query = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);
		// Then set our own desired mode
		mode.dmPelsWidth = width;
		mode.dmPelsHeight = height;
		mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
		ret = ChangeDisplaySettings(&mode , (i < 3 ? CDS_GLOBAL : 0) | (i < 2 ? CDS_UPDATEREGISTRY : 0));
		if (ret == DISP_CHANGE_SUCCESSFUL) break;
		Sleep(1000);
	}
	if (ret != DISP_CHANGE_SUCCESSFUL) {
		char err[200];
		snprintf(err, 200,
			"Fehler beim Setzen der Aufloesung: %d (soll: 0) / %d ( soll: !0)\r\n"
			"Zielaufloesung: %d * %d", ret, query, width, height);
		MessageBoxA(0, err, "OpenSLX", 0);
	}
	return 0;
}

static int muteSound()
{
	CoInitialize(NULL);
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	HRESULT hr = CoCreateInstance(&ID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &ID_IMMDeviceEnumerator, (LPVOID *)&deviceEnumerator);
	if (hr != S_OK) {
		MessageBoxA(0, "CoCreateInstance failed. Cannot mute.", "OpenSLX", 0);
		return 1;
	}
	//deviceEnumerator->lpVtbl->AddRef(deviceEnumerator);
	IMMDevice *defaultDevice = NULL;
	hr = deviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(deviceEnumerator, eRender, eConsole, &defaultDevice);
	if (hr != S_OK) {
		MessageBoxA(0, "GetDefaultAudioEndpoint failed. Cannot mute.", "OpenSLX", 0);
		return 2;
	}
	//defaultDevice->lpVtbl->AddRef(defaultDevice);
	//deviceEnumerator->lpVtbl->Release(deviceEnumerator);
	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->lpVtbl->Activate(defaultDevice, &ID_IAudioEndpointVolume, CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	if (hr != S_OK) {
		MessageBoxA(0, "IMMDevice::Activate() failed. Cannot mute.", "OpenSLX", 0);
		return 3;
	}
	//endpointVolume->lpVtbl->AddRef(endpointVolume);
	//defaultDevice->lpVtbl->Release(defaultDevice);
	float targetVolume = 1;
	endpointVolume->lpVtbl->SetMasterVolumeLevelScalar(endpointVolume, targetVolume, NULL);
	endpointVolume->lpVtbl->SetMute(endpointVolume, TRUE, NULL);
	//endpointVolume->lpVtbl->Release(endpointVolume);
	//CoUninitialize();
	return 0;
}

static int setShutdownText()
{
	HWND hMenu = FindWindowA("DV2ControlHost", NULL);
	if (hMenu == NULL) return 1;
	HWND hPane = FindWindowExA(hMenu, NULL, "DesktopLogoffPane", NULL);
	if (hMenu == NULL) return 2;
	HWND hButton = FindWindowExA(hPane, NULL, "Button", NULL);
	if (hButton == NULL) return 3;
	if (SendMessage(hButton, WM_SETTEXT, 0, (LPARAM)"Abmelden") != TRUE) return 4;
	return 0;
}

