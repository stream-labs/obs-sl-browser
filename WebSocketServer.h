#pragma once

#include <thread>

class WebSocketServer
{
public:
	void start();
	void stop();

	static WebSocketServer &instance()
	{
		static WebSocketServer ws;
		return ws;
	}

private:
	WebSocketServer() {}

	void workerThread();

	bool m_started = false;
	std::thread m_workerThread;
};
