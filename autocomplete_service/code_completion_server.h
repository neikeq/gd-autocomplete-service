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

	struct ClientData {
		Thread *thread;
		Ref<StreamPeerTCP> connection;
		CodeCompletionServer *ccs;
		bool quit;
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

	struct Response {
		Map<String, String> header;
		String status;

		void set_header(const String& p_name, const String& p_value) {
			header.insert(p_name.strip_edges().to_lower(), p_value.strip_edges());
		}
	};

	struct Request {
		ClientData *cd;
		Response response;

		Method method;
		String url;
		String protocol;
		Map<String, String> header;
		int body_size;

		String read_utf8_body() {
			cd->connection->get_utf8_string(body_size);
		}

		void send_response(const String& p_body=String()) {
			String resp = "HTTP/1.1 " + response.status + "\r\n";
			resp += "server: Godot Auto-complete Service\r\n";

			for (Map<String, String>::Element *E=response.header.front(); E; E=E->next()) {
				resp += E->key() + ": " + E->value() + "\r\n";
			}

			if (!response.header.has("connection"))
				resp += "connection: " + header["connection"] + "\r\n";

			resp += "content-length: " + itos(p_body.length()) + "\r\n";
			resp += "\r\n";
			resp += p_body;

			cd->connection->put_utf8_string(resp);
		}
	};

	Ref<TCP_Server> server;
	Set<Thread*> to_wait;

	static void _close_client(ClientData *cd);
	static void _subthread_start(void *s);
	static void _thread_start(void *s);

	Mutex *wait_mutex;
	Thread *thread;
	CodeCompletionService *service;
	Command cmd;
	bool quit;

	int port;
	bool active;

	void _add_to_servers_list();

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
