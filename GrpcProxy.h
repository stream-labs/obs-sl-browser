#pragma once

#include "sl_browser_api.grpc.pb.h"

#include <filesystem>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

class grpc_plugin_objImpl;
class grpc_plugin_objClient;

class GrpcPlugin
{
public:
	static GrpcPlugin &instance()
	{
		static GrpcPlugin a;
		return a;
	}

	bool connectToClient(int32_t port);
	bool startServer(int32_t port);

	void stop();

	grpc_plugin_objClient &getClientObj() const { return *m_clientObj.get(); }

private:
	~GrpcPlugin();

	int32_t m_listenPort{0};

	std::wstring m_modulePath;
	std::unique_ptr<grpc::Server> m_server;
	std::unique_ptr<grpc::ServerBuilder> m_builder;
	std::unique_ptr<grpc_plugin_objImpl> m_serverObj;
	std::unique_ptr<grpc_plugin_objClient> m_clientObj;
};
