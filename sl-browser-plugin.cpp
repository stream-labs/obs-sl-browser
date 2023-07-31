#include <obs.hpp>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-config.h>

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <filesystem>

#include "GrpcPluginServer.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("sl-browser-plugin", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Streamlabs OBS";
}

bool obs_module_load(void)
{
	return true;
}

void obs_module_post_load(void)
{
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	printf("Debugging Window:\n");

	auto chooseProxyPort = []()
	{
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

	int32_t myListenPort = chooseProxyPort();
	int32_t targetListenPort = chooseProxyPort();

	BOOL launched = FALSE;

	PROCESS_INFORMATION slProcessInfo;
	STARTUPINFOW si;
	memset(&si, NULL, sizeof(si));
	si.cb = sizeof(si);

	if (GrpcPluginServer::instance().start(myListenPort)) {

		try {
			const char *module_path = obs_get_module_binary_path(obs_current_module());

			if (!module_path)
				return;

			std::wstring process_path =
				std::filesystem::u8path(module_path).remove_filename().wstring() + L"/sl-browser.exe";

			std::wstring startparams =
				L"sl-browser \"" + std::to_wstring(myListenPort) + L" " + std::to_wstring(targetListenPort);

			launched = CreateProcessW(process_path.c_str(), (LPWSTR)startparams.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
						  NULL, NULL, &si, &slProcessInfo);
		} catch (...) {
			blog(LOG_ERROR, "Streamlabs: obs_module_post_load catch while launching server");
		}
	}

	if (!launched) {
		std::string errorMsg = "Failed to launch " + (std::string)obs_module_description();
		errorMsg += "\nYou may restart the application and try again.";
		::MessageBoxA(NULL, errorMsg.c_str(), "Streamlabs Error", MB_ICONERROR | MB_TOPMOST);
		blog(LOG_ERROR, "Streamlabs: obs_module_post_load can't start vst server, GetLastError = %d", GetLastError());
		return;
	}
}

void obs_module_unload(void)
{

}
