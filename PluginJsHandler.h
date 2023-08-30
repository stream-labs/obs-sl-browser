#pragma once

#include <mutex>
#include <thread>
#include <vector>

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

	void JS_PANEL_EXECUTEJAVASCRIPT(const json11::Json &params);
	void JS_PANEL_SETURL(const json11::Json &params);

	std::mutex m_queueMtx;
	std::atomic<bool> m_running = false;
	std::vector<std::pair<std::string, std::string>> m_queudRequests;
	std::thread m_workerThread;
};
