#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include <obs.h>

#include <json11/json11.hpp>

class PluginJsHandler
{
public:
	void start();
	void stop();
	void pushApiRequest(const std::string &funcName, const std::string &params);
	void executeApiRequest(const std::string &funcName, const std::string &params);

public:
	static PluginJsHandler& instance()
	{
		static PluginJsHandler a;
		return a;
	}

private:
	void workerThread();

	void JS_QUERY_DOCKS(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DOCK_EXECUTEJAVASCRIPT(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DOCK_SETURL(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DOWNLOAD_ZIP(const json11::Json &params, std::string &out_jsonReturn);
	void JS_READ_FILE(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DELETE_FILES(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DROP_FOLDER(const json11::Json &params, std::string &out_jsonReturn);
	void JS_QUERY_DOWNLOADS_FOLDER(const json11::Json &params, std::string &out_jsonReturn);
	void JS_OBS_SOURCE_CREATE(const json11::Json &params, std::string &out_jsonReturn);
	void JS_OBS_SOURCE_DESTROY(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DOCK_SETAREA(const json11::Json &params, std::string &out_jsonReturn);
	void JS_DOCK_NEW_BROWSER_DOCK(const json11::Json &params, std::string &out_jsonReturn);

	std::string getDownloadsDir() const;

	std::mutex m_queueMtx;
	std::atomic<bool> m_running = false;
	std::vector<std::pair<std::string, std::string>> m_queudRequests;
	std::thread m_workerThread;

	std::map<std::string, obs_source_t*> m_sources;

	std::string m_sl_panel_Prefix = "sl_panel_";
};
