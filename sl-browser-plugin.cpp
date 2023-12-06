#include <obs.hpp>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-config.h>
#include <util\platform.h>

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <filesystem>

#include "GrpcPlugin.h"
#include "PluginJsHandler.h"
#include "WebServer.h"
#include "ConsoleToggle.h"
#include "CrashHandler.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolbar>

PROCESS_INFORMATION g_browserProcessInfo;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("sl-browser-plugin", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Streamlabs";
}

bool obs_module_load(void)
{
#ifdef GITHUB_REVISION
	blog(LOG_INFO, "%s module git sha is %s for OBS %s", GITHUB_REVISION, SL_OBS_VERSION);
#else
	blog(LOG_INFO, "%s module is a debug build, version information unknown.");
#endif

	PluginJsHandler::instance().loadFonts();
	PluginJsHandler::instance().loadSlabsBrowserDocks();
	return true;
}

void obs_module_post_load(void)
{
	/***
	* Initializing crash handler information
	*/

	blog(LOG_INFO, "Starting %s plugin...", obs_module_description());

	static auto normalCrash = []() { bcrash(""); };

	CrashHandler::instance().setNormalCrashHandler(normalCrash);
	CrashHandler::instance().setUploadTitle("OBS has crashed!");
	CrashHandler::instance().setUploadMsg("Would you like to upload a detailed report to Streamlabs for further review?");
	CrashHandler::instance().setSentryUri("https://o114354.ingest.sentry.io/api/4506154577756160/minidump/?sentry_key=9860cbedfdebf06a6f209d004b921add");

	// Path to OBS log file, find the most recently updated log file
	std::string logdir = os_get_config_path_ptr("obs-studio/logs/");
	std::filesystem::directory_iterator dir_it(logdir);
	std::filesystem::path latest_file;
	auto latest_time = std::filesystem::file_time_type::min();

	// Find the most recently updated file
	for (const auto &entry : dir_it)
	{
		if (entry.is_regular_file())
		{
			auto last_write_time = entry.last_write_time();
			if (latest_file.empty() || last_write_time > latest_time)
			{
				latest_file = entry.path();
				latest_time = last_write_time;
			}
		}
	}

	CrashHandler::instance().setLogfilePath(latest_file.string());

	/***
	* Plugin begnis
	*/

	PluginJsHandler::instance().start();

	auto chooseProxyPort = []() {
		int32_t result = 0;
		struct sockaddr_in local;
		SOCKET sockt = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

		if (sockt == INVALID_SOCKET)
			return result;

		local.sin_addr.s_addr = htonl(INADDR_ANY);
		local.sin_family = AF_INET;
		local.sin_port = htons(0);

		// Bind
		if (::bind(sockt, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
			return result;

		struct sockaddr_in my_addr;
		memset(&my_addr, NULL, sizeof(my_addr));
		int len = sizeof(my_addr);
		getsockname(sockt, (struct sockaddr *)&my_addr, &len);
		result = ntohs(my_addr.sin_port);

		closesocket(sockt);
		return result;
	};

	bool browserGood = false;
	int32_t myListenPort = chooseProxyPort();
	int32_t targetListenPort = chooseProxyPort();

	blog(LOG_INFO, "%s: Sending %d and %d to proxy", obs_module_description(), myListenPort, targetListenPort);

	STARTUPINFOW si;
	memset(&si, NULL, sizeof(si));
	si.cb = sizeof(si);

	if (GrpcPlugin::instance().startServer(myListenPort))
	{
		try
		{
			const char *module_path = obs_get_module_binary_path(obs_current_module());

			if (!module_path)
				return;

			std::wstring process_path = std::filesystem::u8path(module_path).remove_filename().wstring() + L"/sl-browser.exe";
			std::wstring startparams = L"sl-browser " + std::to_wstring(GetCurrentProcessId()) + L" " + std::to_wstring(myListenPort) + L" " + std::to_wstring(targetListenPort);
			browserGood = CreateProcessW(process_path.c_str(), (LPWSTR)startparams.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &g_browserProcessInfo);
		}
		catch (...)
		{
			blog(LOG_ERROR, "%s: obs_module_post_load catch while launching server", obs_module_description());
		}

		if (browserGood && !GrpcPlugin::instance().connectToClient(targetListenPort))
		{
			browserGood = FALSE;
			blog(LOG_ERROR, "%s: obs_module_post_load can't connect to process, GetLastError = %d", obs_module_description(), GetLastError());

			// Terminates the process
			TerminateProcess(g_browserProcessInfo.hProcess, EXIT_SUCCESS);
			CloseHandle(g_browserProcessInfo.hProcess);
			CloseHandle(g_browserProcessInfo.hThread);
		}
	}

	std::string errorMsg = "Failed to initialize plugin " + std::string(obs_module_description()) + "\nRestart the application and try again.";

	if (!browserGood)
	{
		::MessageBoxA(NULL, errorMsg.c_str(), "Streamlabs Error", MB_ICONERROR | MB_TOPMOST);
		blog(LOG_ERROR, "Streamlabs: obs_module_post_load can't start proxy process, GetLastError = %d", GetLastError());
		return;
	}

	// Streamlabs button to toggle visibility
	QMainWindow * window = (QMainWindow *)obs_frontend_get_main_window();
	QAction *action = new QAction("Streamlabs", window);
	QObject::connect(action, &QAction::triggered, [=]() {

		// Has to be run in a seperate thread because bringing to foreground will take over input msg and we're in the middle of using it
		std::thread([&]() { GrpcPlugin::instance().getClient()->send_windowToggleVisibility(); }).detach();
	});

	window->menuBar()->addAction(action);
}

void obs_module_unload(void)
{
	// Tell process to shut down and wait?
	// Might be fine to just kill it, tbd
	// ;

	PluginJsHandler::instance().stop();
	GrpcPlugin::instance().stop();
	WebServer::instance().stop();

	// Terminates the process (it shouldn't exist)
	TerminateProcess(g_browserProcessInfo.hProcess, EXIT_SUCCESS);
	CloseHandle(g_browserProcessInfo.hProcess);
	CloseHandle(g_browserProcessInfo.hThread);
}
