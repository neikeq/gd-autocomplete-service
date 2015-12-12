/* code_completion_server.h */
#ifndef CODE_COMPLETION_SERVER_H
#define CODE_COMPLETION_SERVER_H

#include "object.h"
#include "os/thread.h"
#include "io/tcp_server.h"
#include "code_completion_service.h"

class CodeCompletionServer : public Object {

	OBJ_TYPE(CodeCompletionServer, Object);

	enum Command {
		CMD_NONE,
		CMD_ACTIVATE,
		CMD_STOP,
	};

	enum Method {
		METHOD_GET = 0,
		METHOD_HEAD,
		METHOD_POST,
		METHOD_PUT,
		METHOD_DELETE,
		METHOD_OPTIONS,
		METHOD_TRACE,
		METHOD_CONNECT,
		METHOD_MAX
	};

	struct ClientData {
		Thread *thread;
		Ref<StreamPeerTCP> connection;
		CodeCompletionServer *ccs;
		bool quit;
	};

	Ref<TCP_Server> server;
	Set<Thread*> to_wait;

	static void _close_client(ClientData *cd);
	static void _subthread_start(void *s);
	static void _thread_start(void *s);

	static Error _write_response(ClientData *cd, const String& p_status, const String p_headers, const String& p_body, bool p_close=false);

	Mutex *wait_mutex;
	Thread *thread;
	CodeCompletionService *service;
	Command cmd;
	bool quit;

	int port;
	bool active;

public:

	void start(CodeCompletionService *r_service);
	void stop();

	bool is_active() const;

	_FORCE_INLINE_ CodeCompletionService* get_service() {
		return service;
	}

	CodeCompletionServer();
	~CodeCompletionServer();
};

#endif // CODE_COMPLETION_SERVER_H
