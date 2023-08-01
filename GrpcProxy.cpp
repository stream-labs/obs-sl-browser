#include "GrpcProxy.h"

#include <filesystem>

/***
* Server
* Receiving messages from the proxy
*/

class grpc_proxy_objImpl final : public grpc_proxy_obj::Service{
	grpc::Status com_grpc_dispatcher(grpc::ServerContext *context, const grpc_example_Request *request, grpc_example_Reply *response) override
	{
		return grpc::Status::OK;
	}
};

/***
* Client
* Sending messages to the proxy
*/

class grpc_proxy_objClient {
public:
	grpc_proxy_objClient(std::shared_ptr<grpc::Channel> channel) : stub_(grpc_proxy_obj::NewStub(channel))
	{
		m_connected = channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(3));
	}

	bool do_grpc_example_Request(int param1)
	{
		grpc_example_Request request;
		request.set_param1(param1);

		grpc_example_Reply reply;
		grpc::ClientContext context;
		grpc::Status status = stub_->com_grpc_dispatcher(&context, request, &reply);

		if (!status.ok())
			return m_connected = false;

		printf("Got back %lld\n", reply.param1());

		return true;
	}

	std::atomic<bool> m_connected{false};

private:
	std::unique_ptr<grpc_proxy_obj::Stub> stub_;
};

// Grpc
//

GrpcProxy::GrpcProxy() {

}

GrpcProxy::~GrpcProxy() {

}

bool GrpcProxy::startServer(int32_t listenPort)
{
	// Don't repeat
	if (m_listenPort != 0)
		return false;

	m_listenPort = listenPort;

	grpc::EnableDefaultHealthCheckService(true);
	grpc::reflection::InitProtoReflectionServerBuilderPlugin();

	m_builder = std::make_unique<grpc::ServerBuilder>();
	m_builder->AddListeningPort(std::string("localhost:") + std::to_string(m_listenPort), grpc::InsecureServerCredentials());

	m_serverObj = std::make_unique<grpc_proxy_objImpl>();
	m_builder->RegisterService(m_serverObj.get());

	m_server = m_builder->BuildAndStart();

	return m_server != nullptr;
}

bool GrpcProxy::connectToClient(int32_t portNumber)
{
	m_clientObj = std::make_unique<grpc_proxy_objClient>(
		grpc::CreateChannel("localhost:" + std::to_string(portNumber), grpc::InsecureChannelCredentials()));

	return m_clientObj != nullptr;
}

void GrpcProxy::stop() {

}
