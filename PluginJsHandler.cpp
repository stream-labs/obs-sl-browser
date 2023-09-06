#include "PluginJsHandler.h"
#include "JavascriptApi.h"
#include "GrpcPlugin.h"
#include "BrowserDockCaster.h"
#include "cef-headers.hpp"
#include "deps/minizip/unzip.h"

#include <wininet.h>
#include <chrono>
#include <fstream>
#include <ShlObj.h>

#include <obs.h>
#include <obs-frontend-api.h>

#include <QMainWindow>
#include <QDockWidget>

#include "../obs-browser/panel/browser-panel-internal.hpp"

#pragma comment(lib, "wininet.lib")

void PluginJsHandler::start()
{
	m_running = true;
	m_workerThread = std::thread(&PluginJsHandler::workerThread, this);
}

void PluginJsHandler::stop()
{
	m_running = false;

	if (m_workerThread.joinable())
		m_workerThread.join();
}

void PluginJsHandler::pushApiRequest(const std::string &funcName, const std::string &params)
{
	std::lock_guard<std::mutex> grd(m_queueMtx);
	m_queudRequests.push_back({funcName, params});
}

std::string PluginJsHandler::getDownloadsDir() const
{
	char path[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
		return std::string(path) + "\\slabsdownloads";

	return "";
}

void PluginJsHandler::executeApiRequest(const std::string& funcName, const std::string& params)
{
	std::string err;
	json11::Json jsonParams = json11::Json::parse(params, err);

	if (!err.empty())
	{
		blog(LOG_ERROR, "PluginJsHandler::executeApiRequest invalid params %s", params.c_str());
		return;
	}

	const auto &param1Value = jsonParams["param1"];

	if (param1Value.is_null())
	{
		blog(LOG_ERROR, "PluginJsHandler::executeApiRequest Error: 'param1' key not found. %s", params.c_str());
		return;
	}

	std::string jsonReturnStr;

	switch (JavascriptApi::getFunctionId(funcName)) {
	case JavascriptApi::JS_PANEL_EXECUTEJAVASCRIPT: {
		JS_PANEL_EXECUTEJAVASCRIPT(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_PANEL_SETURL: {
		JS_PANEL_SETURL(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_QUERY_PANELS: {
		JS_QUERY_PANELS(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_DOWNLOAD_ZIP: {
		JS_DOWNLOAD_ZIP(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_READ_FILE: {
		JS_READ_FILE(jsonParams, jsonReturnStr);
		break;
	}
	}

	// We're done, send callback
	if (param1Value.int_value() > 0)
		GrpcPlugin::instance().getClient()->send_executeCallback(param1Value.int_value(), jsonReturnStr);
}

void PluginJsHandler::workerThread()
{
	while (m_running)
	{
		std::vector<std::pair<std::string, std::string>> latestBatch;

		{
			std::lock_guard<std::mutex> grd(m_queueMtx);
			latestBatch.swap(m_queudRequests);
		}

		if (latestBatch.empty())
		{
			using namespace std::chrono;
			std::this_thread::sleep_for(1ms);
		}
		else
		{
			for (auto &itr : latestBatch)
				executeApiRequest(itr.first, itr.second);
		}
	}
}

void PluginJsHandler::JS_QUERY_PANELS(const json11::Json &params, std::string &out_jsonReturn)
{
	blog(LOG_WARNING, "JS_QUERY_PANELS start: %s\n", params.dump().c_str());

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	QMetaObject::invokeMethod(
		mainWindow,
		[mainWindow, &out_jsonReturn]() {
			std::vector<json11::Json> panelInfo;

			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid()) {
					QCefWidgetInternal *widget = (QCefWidgetInternal *)BrowserDockCaster::getQCefWidget(dock);

					if (widget->cefBrowser != nullptr) {
						std::string uuid = dock->property("uuid").toString().toStdString();
						std::string url = widget->cefBrowser->GetMainFrame()->GetURL();

						// Create a json11::Json object for this dock widget and add it to the panelInfo vector
						panelInfo.push_back(json11::Json::object{{"uuid", uuid}, {"url", url}});
					}
				}
			}

			// Convert the panelInfo vector to a json11::Json object and dump string
			json11::Json ret = panelInfo;
			out_jsonReturn = ret.dump();
		},
		Qt::BlockingQueuedConnection);


	blog(LOG_WARNING, "JS_QUERY_PANELS output: %s\n", out_jsonReturn.c_str());
}

void PluginJsHandler::JS_PANEL_EXECUTEJAVASCRIPT(const json11::Json &params, std::string &out_jsonReturn)
{
	blog(LOG_WARNING, "JS_PANEL_EXECUTEJAVASCRIPT: %s\n", params.dump().c_str());

	const auto &param2Value = params["param2"];
	const auto &param3Value = params["param3"];

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	// This code is executed in the context of the QMainWindow's thread.
	QMetaObject::invokeMethod(
		mainWindow,
		[mainWindow, param2Value, param3Value, &out_jsonReturn]() {
			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid() &&
				    dock->property("uuid").toString().toStdString() == param3Value.string_value()) {
					QCefWidgetInternal *widcket = (QCefWidgetInternal *)BrowserDockCaster::getQCefWidget(dock);
					widcket->executeJavaScript(param2Value.string_value().c_str());
					return;
				}
			}

			std::string errorMsg = "Dock '" + param2Value.string_value() + "' not found.";
			json11::Json ret = json11::Json::object({{"error", errorMsg}});
			out_jsonReturn = ret.dump();
		},
		Qt::BlockingQueuedConnection);

	blog(LOG_WARNING, "JS_PANEL_EXECUTEJAVASCRIPT output: %s\n", out_jsonReturn.c_str());
}

void PluginJsHandler::JS_PANEL_SETURL(const json11::Json &params, std::string &out_jsonReturn)
{
	blog(LOG_WARNING, "JS_PANEL_SETURL: %s\n", params.dump().c_str());

	const auto &param2Value = params["param2"];
	const auto &param3Value = params["param3"];

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	// This code is executed in the context of the QMainWindow's thread.
	QMetaObject::invokeMethod(mainWindow,
		[mainWindow, param2Value, param3Value, &out_jsonReturn]() {
			
			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid() &&
				    dock->property("uuid").toString().toStdString() == param3Value.string_value())
				{
					QCefWidgetInternal *widcket = (QCefWidgetInternal *)BrowserDockCaster::getQCefWidget(dock);
					widcket->setURL(param2Value.string_value().c_str());
					return;
				}
			}

			std::string errorMsg = "Dock '" + param2Value.string_value() + "' not found.";
			json11::Json ret = json11::Json::object({{"error", errorMsg}});
			out_jsonReturn = ret.dump();
		},
		Qt::BlockingQueuedConnection);

	blog(LOG_WARNING, "JS_PANEL_SETURL output: %s\n", out_jsonReturn.c_str());
}

void PluginJsHandler::JS_DOWNLOAD_ZIP(const json11::Json &params, std::string &out_jsonReturn)
{
	auto downloadFile = [](const std::string &url, const std::string &filename)
	{
		HINTERNET connect = InternetOpenA("Streamlabs", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

		if (!connect)
			return false;

		HINTERNET hOpenAddress =
			InternetOpenUrlA(connect, url.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);

		if (!hOpenAddress)
		{
			InternetCloseHandle(connect);
			return false;
		}

		DWORD contentLength;
		DWORD contentLengthSize = sizeof(contentLength);
		if (!HttpQueryInfo(hOpenAddress, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, NULL))
		{
			blog(LOG_ERROR, "JS_DOWNLOAD_ZIP: HttpQueryInfo failed, last error = %d\n", GetLastError());
			InternetCloseHandle(hOpenAddress);
			InternetCloseHandle(connect);
			return false;
		}

		char dataReceived[4096];
		DWORD numberOfBytesRead = 0;

		std::ofstream outFile(filename, std::ios::binary);
		if (!outFile.is_open())
		{
			blog(LOG_ERROR, "JS_DOWNLOAD_ZIP: Could not open std::ofstream outFile, last error = %d\n", GetLastError());
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
			blog(LOG_ERROR, "JS_DOWNLOAD_ZIP: Incomplete download, last error = %d\n", GetLastError());
			std::remove(filename.c_str());
			return false;
		}

		return true;
	};

	auto unzip = [](const std::string &filepath, std::vector<std::string> &output)
	{
		unzFile zipFile = unzOpen(filepath.c_str());
		if (!zipFile) {
			blog(LOG_ERROR, "Unable to open zip file: %s", filepath.c_str());
			return;
		}

		std::filesystem::path outputDir = std::filesystem::path(filepath).parent_path();
		char buffer[4096];

		unz_global_info globalInfo;
		if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK) {
			blog(LOG_ERROR, "Could not read file global info: %s", filepath.c_str());
			unzClose(zipFile);
			return;
		}

		for (uLong i = 0; i < globalInfo.number_entry; ++i) {
			unz_file_info fileInfo;
			char filename[256];
			if (unzGetCurrentFileInfo(zipFile, &fileInfo, filename, 256, NULL, 0, NULL, 0) != UNZ_OK) {
				blog(LOG_ERROR, "Could not read file info: %s", filepath.c_str());
				unzClose(zipFile);
				return;
			}

			const std::string fullOutputPath = outputDir.string() + '/' + filename;

			// If the file in the zip archive is a directory, continue to next file
			if (filename[strlen(filename) - 1] == '/') {
				if ((i + 1) < globalInfo.number_entry) {
					if (unzGoToNextFile(zipFile) != UNZ_OK) {
						blog(LOG_ERROR, "Could not read next file: %s", filepath.c_str());
						unzClose(zipFile);
						return;
					}
				}
				continue;
			}

			if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
				blog(LOG_ERROR, "Could not open current file: %s", filepath.c_str());
				unzClose(zipFile);
				return;
			}

			// Create necessary directories
			std::filesystem::path pathToFile(fullOutputPath);
			if (fileInfo.uncompressed_size > 0) {
				std::filesystem::create_directories(pathToFile.parent_path());
			}

			std::ofstream outFile(fullOutputPath, std::ios::binary);
			int error = UNZ_OK;
			do {
				error = unzReadCurrentFile(zipFile, buffer, sizeof(buffer));
				if (error < 0) {
					blog(LOG_ERROR, "Error %d with zipfile in unzReadCurrentFile: %s", error, filepath.c_str());
					break;
				}
				if (error > 0) {
					outFile.write(buffer, error);
				}
			} while (error > 0);

			outFile.close();

			// Adding the file path to the output vector
			output.push_back(fullOutputPath);

			if (unzCloseCurrentFile(zipFile) != UNZ_OK) {
				blog(LOG_ERROR, "Could not close file: %s", filepath.c_str());
			}

			if ((i + 1) < globalInfo.number_entry) {
				if (unzGoToNextFile(zipFile) != UNZ_OK) {
					blog(LOG_ERROR, "Could not read next file: %s", filepath.c_str());
					unzClose(zipFile);
					return;
				}
			}
		}

		unzClose(zipFile);
	};
	
	const auto &param2Value = params["param2"];
	std::string url = param2Value.string_value();
	std::string folderPath = getDownloadsDir();

	if (!folderPath.empty())
	{
		using namespace std::chrono;

		// Current time in miliseconds
		system_clock::time_point now = system_clock::now();
		auto duration = now.time_since_epoch();
		auto millis = duration_cast<milliseconds>(duration).count();
		std::string millis_str = std::to_string(millis);

		// ThreadID + MsTime should be unique, same threaID within 1ms window is a statistical improbability 
		std::string subFolderPath = folderPath + "\\" + std::to_string(GetCurrentThreadId()) + millis_str;
		std::string zipFilepath = subFolderPath + "\\download.zip";

		CreateDirectoryA(folderPath.c_str(), NULL);
		CreateDirectoryA(subFolderPath.c_str(), NULL);

		if (downloadFile(url, zipFilepath))
		{
			std::vector<std::string> filepaths;
			unzip(zipFilepath, filepaths);

			// Build json string now
			json11::Json::array json_array;
    
			for (const auto& filepath : filepaths) {
				json11::Json::object obj;
				obj["path"] = filepath;
				json_array.push_back(obj);
			}

			json11::Json json_object = json11::Json(json_array);
			out_jsonReturn = json_object.dump();
		}
		else
		{
			json11::Json ret = json11::Json::object({{"error", "Http download file failed"}});
			out_jsonReturn = ret.dump();
		}

		// zip file itself not needed
		std::remove(zipFilepath.c_str());
	}
	else
	{
		json11::Json ret = json11::Json::object({{"error", "File system can't access Local AppData folder"}});
		out_jsonReturn = ret.dump();
	}

	blog(LOG_WARNING, "JS_DOWNLOAD_ZIP output: %s\n", out_jsonReturn.c_str());
}

void PluginJsHandler::JS_READ_FILE(const json11::Json &params, std::string &out_jsonReturn)
{
	const auto &param2Value = params["param2"];

	std::string filepath = param2Value.string_value();
	std::string filecontents;

	std::ifstream file(filepath, std::ios::binary | std::ios::ate);

	json11::Json ret;

	if (file) {
		try {

			// Get the file size
			std::streamsize fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			// Check if file size is 1MB or higher
			if (fileSize >= 1048576) {
				ret = json11::Json::object({{"error", "File size is 1MB or higher"}});
			} else {
				std::stringstream buffer;
				buffer << file.rdbuf();
				filecontents = buffer.str();
				ret = json11::Json::object({{"contents", filecontents}});
			}
		} catch (...) {
			ret = json11::Json::object(
				{{"error", "Unable to read file. Checking for windows errors: '" + std::to_string(GetLastError()) + "'"}});
		}
	} else {
		ret = json11::Json::object(
			{{"error", "Unable to open file. Checking for windows errors: '" + std::to_string(GetLastError()) + "'"}});
	}

	out_jsonReturn = ret.dump();
}
