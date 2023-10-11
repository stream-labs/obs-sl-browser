#pragma once

#include <string>
#include <windows.h>
#include <wininet.h>
#include <fstream>

#include "deps/minizip/unzip.h"

#pragma comment(lib, "wininet.lib")

namespace Util
{
	static void ForceForegroundWindow(HWND focusOnWindowHandle)
	{
		int style = GetWindowLong(focusOnWindowHandle, GWL_STYLE);

		// Minimize and restore to be able to make it active.
		if ((style & WS_MINIMIZE) == WS_MINIMIZE)
			ShowWindow(focusOnWindowHandle, SW_RESTORE);

		auto currentlyFocusedWindowProcessId = GetWindowThreadProcessId(GetForegroundWindow(), LPDWORD(0));
		auto appThread = GetCurrentThreadId();

		if (currentlyFocusedWindowProcessId != appThread)
		{
			AttachThreadInput(currentlyFocusedWindowProcessId, appThread, true);
			BringWindowToTop(focusOnWindowHandle);
			AttachThreadInput(currentlyFocusedWindowProcessId, appThread, false);
		}

		else
		{
			BringWindowToTop(focusOnWindowHandle);
		}
	}

	static bool Unzip(const std::string &filepath, std::vector<std::string> &output)
	{
		unzFile zipFile = unzOpen(filepath.c_str());
		if (!zipFile)
		{
			// Failed to open the zip file
			return false;
		}

		unz_global_info globalInfo;
		if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK)
		{
			unzClose(zipFile);
			return false;
		}

		char buffer[4096]; // Adjust buffer size as needed

		for (uLong i = 0; i < globalInfo.number_entry; ++i)
		{
			char filename[256];
			unz_file_info fileInfo;

			if (unzGetCurrentFileInfo(zipFile, &fileInfo, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK)
			{
				unzClose(zipFile);
				return false;
			}

			std::filesystem::path fullOutputPath = std::filesystem::path(filepath).parent_path() / filename;

			if (filename[strlen(filename) - 1] == '/')
			{
				// It's a directory
				std::filesystem::create_directories(fullOutputPath);
			}
			else
			{
				// It's a file
				if (unzOpenCurrentFile(zipFile) != UNZ_OK)
				{
					unzClose(zipFile);
					return false;
				}

				std::filesystem::create_directories(fullOutputPath.parent_path());
				std::ofstream outFile(fullOutputPath, std::ios::binary);
				int error = UNZ_OK;
				do
				{
					error = unzReadCurrentFile(zipFile, buffer, sizeof(buffer));
					if (error < 0)
					{
						unzCloseCurrentFile(zipFile);
						unzClose(zipFile);
						return false;
					}
					if (error > 0)
					{
						outFile.write(buffer, error);
					}
				} while (error > 0);

				outFile.close();
				output.push_back(fullOutputPath.string());

				if (unzCloseCurrentFile(zipFile) != UNZ_OK)
				{
					unzClose(zipFile);
					return false;
				}
			}

			if ((i + 1) < globalInfo.number_entry)
			{
				if (unzGoToNextFile(zipFile) != UNZ_OK)
				{
					unzClose(zipFile);
					return false;
				}
			}
		}

		unzClose(zipFile);
		return true;
	}

	static bool DownloadFile(const std::string &url, const std::string &filename)
	{
		HINTERNET connect = InternetOpenA("Streamlabs", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

		if (!connect)
			return false;

		HINTERNET hOpenAddress = InternetOpenUrlA(connect, url.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);

		if (!hOpenAddress)
		{
			InternetCloseHandle(connect);
			return false;
		}

		DWORD contentLength;
		DWORD contentLengthSize = sizeof(contentLength);
		if (!HttpQueryInfo(hOpenAddress, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, NULL))
		{
			//blog(LOG_ERROR, "DownloadFile: HttpQueryInfo failed, last error = %d\n", GetLastError());
			InternetCloseHandle(hOpenAddress);
			InternetCloseHandle(connect);
			return false;
		}

		char dataReceived[4096];
		DWORD numberOfBytesRead = 0;

		std::ofstream outFile(filename, std::ios::binary);
		if (!outFile.is_open())
		{
			//blog(LOG_ERROR, "DownloadFile: Could not open std::ofstream outFile, last error = %d\n", GetLastError());
			InternetCloseHandle(hOpenAddress);
			InternetCloseHandle(connect);
			return false;
		}

		DWORD totalBytesRead = 0;
		while (InternetReadFile(hOpenAddress, dataReceived, 4096, &numberOfBytesRead) && numberOfBytesRead > 0)
		{
			outFile.write(dataReceived, numberOfBytesRead);
			totalBytesRead += numberOfBytesRead;
		}

		outFile.close();
		InternetCloseHandle(hOpenAddress);
		InternetCloseHandle(connect);

		if (totalBytesRead != contentLength)
		{
			//blog(LOG_ERROR, "DownloadFile: Incomplete download, last error = %d\n", GetLastError());
			std::remove(filename.c_str());
			return false;
		}

		return true;
	};

	static bool InstallFont(const char *fontPath)
	{
		if (AddFontResourceA(fontPath))
		{
			SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
			return true;
		}

		//blog(LOG_ERROR, "InstallFont: Failed, last error = %d\n", GetLastError());
		return false;
	}

}
