#include <windows.h>
#include "out.h" // winamp sdk (output plugin)

#include <pulse/simple.h>
#include <pulse/error.h>

#include "main.h"
//#define _DEBUG
#include "debug.h"

#define PI_VER "v0.1(b)"

#define BUFF_SIZE 8192 // default buffer size

extern Out_Module out_module;

// Audio device server // NULL = Default
char g_server_buff[128] = "127.0.0.1";
char * g_server = NULL;
int g_servertype = 0; // 0 = local; 1 = remote;

// Format
pa_simple * s = NULL;
int g_srate;
int g_numchan;
int g_bps;

// Time
int g_flush_time = 0;
int g_open_time = 0;

// Play
int g_pause;
int g_volume = 255;
int g_volume_control = 1;
int g_pan = 0;
int g_pan_control = 1;

void config_load()
{
	char cfg_path[MAX_PATH];
	GetModuleFileName(out_module.hDllInstance, cfg_path, sizeof(cfg_path));
	char * cfg_p = strrchr(cfg_path, '.');
	if (cfg_p)
	{
		strcpy(cfg_p, ".cfg");
		g_servertype = GetPrivateProfileInt("PulseAudio", "ServerType", 0, cfg_path);
		GetPrivateProfileString("PulseAudio", "ServerName", "127.0.0.1", g_server_buff, sizeof(g_server_buff), cfg_path);
		g_volume_control = GetPrivateProfileInt("PulseAudio", "VolumeControl", 1, cfg_path);
		g_pan_control = GetPrivateProfileInt("PulseAudio", "PanControl", 1, cfg_path);
	}
}

void config_save()
{
	char cfg_path[MAX_PATH];
	GetModuleFileName(out_module.hDllInstance, cfg_path, sizeof(cfg_path));
	char * cfg_p = strrchr(cfg_path, '.');
	if (cfg_p)
	{
		strcpy(cfg_p, ".cfg");
		WritePrivateProfileString("PulseAudio", "ServerType", g_servertype?"1":"0", cfg_path);
		WritePrivateProfileString("PulseAudio", "ServerName", g_server_buff, cfg_path);
		WritePrivateProfileString("PulseAudio", "VolumeControl", g_volume_control?"1":"0", cfg_path);
		WritePrivateProfileString("PulseAudio", "PanControl", g_pan_control?"1":"0", cfg_path);
	}
}

BOOL CALLBACK configDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			CheckDlgButton(hDlg, ID_CB_VOLUME, g_volume_control?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hDlg, ID_CB_PAN, g_pan_control?BST_CHECKED:BST_UNCHECKED);
			if (g_servertype)
			{
				CheckDlgButton(hDlg, ID_RB_REMOTESERVER, BST_CHECKED);
			} else
			{
				CheckDlgButton(hDlg, ID_RB_LOCALSERVER, BST_CHECKED);
			}
			SetDlgItemTextA(hDlg, ID_ED_SERVER, g_server_buff);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					g_volume_control = IsDlgButtonChecked(hDlg, ID_CB_VOLUME) == BST_CHECKED ?1:0;
					g_pan_control = IsDlgButtonChecked(hDlg, ID_CB_PAN) == BST_CHECKED ?1:0;
					if (IsDlgButtonChecked(hDlg, ID_RB_LOCALSERVER) == BST_CHECKED)
					{
						g_servertype = 0;
					} else
					{
						g_servertype = 1;
					}
					GetDlgItemTextA(hDlg, ID_ED_SERVER, g_server_buff, sizeof(g_server_buff));
					g_server = g_server_buff;
					PostMessageA(hDlg, WM_CLOSE, 0, 0);
					break;
				case IDCANCEL:
					PostMessageA(hDlg, WM_CLOSE, 0, 0);
					break;
			}
			break;
		case WM_CLOSE:
			EndDialog(hDlg, 0);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void config(HWND hwndParent) // configuration dialog
{
	//dprintf("config");
	DialogBoxA(out_module.hDllInstance, MAKEINTRESOURCEA(DLG_CONFIG), hwndParent, configDlgProc);
}

void about(HWND hwndParent)  // about dialog
{
	//dprintf("about");
	MessageBoxA(hwndParent,
		"PulseAudio Output Plug-In " PI_VER "\n"
		"Copyright (c) 2010 BulldozerBSG",
		"About",
		0);
}

void init()					// called when loaded
{
	//dprintf("init");
	config_load();
	if (g_servertype == 1)
	{
		g_server = g_server_buff;
	} else
	{
		g_server = NULL;
	}
}

void quit()					// called when unloaded
{
	//dprintf("quit");
	config_save();
}

int open(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms)
					// returns >=0 on success, <0 on failure

					// NOTENOTENOTE: bufferlenms and prebufferms are ignored in most if not all output plug-ins. 
					//    ... so don't expect the max latency returned to be what you asked for.
					// returns max latency in ms (0 for diskwriters, etc)
					// bufferlenms and prebufferms must be in ms. 0 to use defaults. 
					// prebufferms must be <= bufferlenms
					// pass bufferlenms==-666 to tell the output plugin that it's clock is going to be used to sync video
					//   out_ds turns off silence-eating when -666 is passed
{
	//dprintf("open");
	//dprintf("open: bufferlenms(%d) prebufferms(%d)", bufferlenms, prebufferms);
	pa_sample_spec ss = {0};
	g_flush_time = 0;
	g_open_time = GetTickCount();
	g_numchan = numchannels;
	g_srate = samplerate;
	g_bps = bitspersamp;
	ss.rate = samplerate;
	ss.channels = numchannels;
	switch (bitspersamp)
	{
		case 16:
		{
			ss.format = PA_SAMPLE_S16NE;
			break;
		}
		default:
		{
			//dprintf("VARNING(open): Unsuported format");
			MessageBoxA(out_module.hMainWindow,
				"Not supported audio format.",
				"PulseAudio: VARNING!",
				0);
			return -1;
			break;
		}
	}
	s = pa_simple_new(g_server,
		"Winamp",
		PA_STREAM_PLAYBACK,
		NULL,
		"music",
		&ss,
		NULL,
		NULL,
		NULL);
	if (s)
	{

		g_open_time = GetTickCount();
		return 0;
	}
	//dprintf("VARNING(open): Not open device");
	MessageBoxA(out_module.hMainWindow,
		"Can not open audio device.",
		"PulseAudio: VARNING!",
		0);
	return -1;
}

void close()				// close the ol' output device.
{
	//dprintf("close");
	if (s)
	{
		int error = 0;
		//pa_simple_drain(s, &error);
		pa_simple_free(s);
		s = NULL;
	}
}

int write(char *buf, int len)
					// 0 on success. Len == bytes to write (<= 8192 always). buf is straight audio data. 
					// 1 returns not able to write (yet). Non-blocking, always.
{
	//dprintf("write");
	int result = 1;
	if (s && len)
	{
		int error = 0;
		if ((g_volume_control && g_volume < 255) || (g_pan_control && g_pan != 0))
		{
			if (g_bps == 16)
			{
				short int * buff = (short int *)buf;
				int cur_chan = 1;
				while (buff < (short int *)(buf + len))
				{
					if (g_volume_control && g_volume < 255)
					{
						*buff = *buff * g_volume / 255;
					}
					if (g_numchan == 2 && g_pan_control && g_pan != 0)
					{
						if (cur_chan == 1) // l
						{
							int l = g_pan <= 0 ?128:128-g_pan;
							*buff = *buff * l / 128;
							cur_chan = 2;
						} else // r
						{
							int r = g_pan >= 0 ?128:128+g_pan;
							*buff = *buff * r / 128;
							cur_chan = 1;
						}
					}
					buff++;
				}
			}
		}
		pa_simple_write(s, buf, len, &error);
		result = 0;
	}
	return result;
}

int canwrite()				// returns number of bytes possible to write at a given time. 
							// Never will decrease unless you call Write (or Close, heh)
{
	//dprintf("canwrite");
	if (g_pause)
	{
		return 0;
	}
	return BUFF_SIZE;
}

int isplaying()				// non 0 if output is still going or if data in buffers waiting to be
							// written (i.e. closing while IsPlaying() returns 1 would truncate the song
{
	//dprintf("isplaying");
	return 0;
}

int pause(int pause)		// returns previous pause state
{
	//dprintf("pause");
	int previous_pause = g_pause;
	if (!g_pause && pause)
	{
		g_flush_time += GetTickCount() - g_open_time;
	}
	if (g_pause && !pause)
	{
		g_open_time = GetTickCount();
	}
	g_pause = pause;
	return previous_pause;
}

void setvolume(int volume)	// volume is 0-255
{
	//dprintf("setvolume");
	//dprintf("setvolume: %d", volume);
	if (volume < 0 || g_volume > 255)
		return;
	g_volume = volume;
}

void setpan(int pan)		// pan is -128 to 128
{
	//dprintf("setpan");
	//dprintf("setpan: %d", pan);
	g_pan = pan;
}

void flush(int t)			// flushes buffers and restarts output at time t (in ms) 
							// (used for seeking)
{
	//dprintf("flush");
	g_flush_time = t;
	g_open_time = GetTickCount();
	if (s)
	{
		int error = 0;
		pa_simple_flush(s, &error);
	}
}

int getoutputtime()			// returns played time in MS
{
	//dprintf("getoutputtime");
	if (g_pause)
	{
		return g_flush_time;
	} else
	{
		return GetTickCount() - g_open_time + g_flush_time;
	}
}

int getwrittentime()		// returns time written in MS (used for synching up vis stuff)
{
	//dprintf("getwrittentime");
	if (g_pause)
	{
		return g_flush_time;
	} else
	{
		return GetTickCount() - g_open_time + g_flush_time;
	}
}

Out_Module out_module = {
	OUT_VER,
	"PulseAudio Output Plug-In " PI_VER,
	77777,
	NULL, // hmainwindow
	NULL, // hdllinstance
	config,
	about,
	init,
	quit,
	open,
	close,
	write,
	canwrite,
	isplaying,
	pause,
	setvolume,
	setpan,
	flush,
	getoutputtime,
	getwrittentime
};

__declspec(dllexport) Out_Module * winampGetOutModule()
{
	//dprintf("winampGetOutModule");
	return &out_module;
}
