/* code_completion_server.cpp */
#include "code_completion_server.h"
#include "os/file_access.h"
#include "tools/editor/editor_settings.h"
#include "globals.h"

#define CLOSE_CLIENT_COND(m_cond, m_cd) \
	{ if ( m_cond ) {	\
        _close_client(m_cd);	\
		return;	 \
	} }	\

void CodeCompletionServer::_close_client(ClientData *cd) {

	cd->connection->disconnect();
	cd->ccs->wait_mutex->lock();
	cd->ccs->to_wait.insert(cd->thread);
	cd->ccs->wait_mutex->unlock();
	memdelete(cd);
}

void CodeCompletionServer::_subthread_start(void *s) {

	ClientData *cd = (ClientData*)s;
	cd->connection->set_nodelay(true);

	static const char* _methods[METHOD_MAX]={
		"GET",
		"HEAD",
		"POST",
		"PUT",
		"DELETE",
		"OPTIONS",
		"TRACE",
		"CONNECT"
	};

	Vector<uint8_t> request_str;

	while(!cd->quit) {
		uint8_t byte;
		Error err = cd->connection->get_data(&byte, 1);
		CLOSE_CLIENT_COND(err!=OK, cd);

		request_str.push_back(byte);
		int rs = request_str.size();

		if ((rs>=2 && request_str[rs-2]=='\n' && request_str[rs-1]=='\n') ||
			(rs>=4 && request_str[rs-4]=='\r' && request_str[rs-3]=='\n' && rs>=4 && request_str[rs-2]=='\r' && request_str[rs-1]=='\n')) {

			request_str.push_back(0);

			// End of request header, parse
			String header;
			CLOSE_CLIENT_COND(header.parse_utf8((const char*)request_str.ptr()), cd);
			Vector<String> header_lines = header.split("\n");
			request_str.clear();

			Request request;

			for (int i = 0; i < header_lines.size(); i++) {
				String s = header_lines[i].strip_edges();
				if (s.length() == 0)
					continue;

				if (i == 0) {
					// Parse command, url and protocol
					Vector<String> parts = s.split(" ");
					CLOSE_CLIENT_COND(parts.size() < 3, cd);

					int method_idx = -1;
					for (int j = 0; j < METHOD_MAX; j++) {
						if (_methods[j] == parts[0]) {
							method_idx = j;
							break;
						}
					}
					CLOSE_CLIENT_COND(method_idx < 0, cd);

					request.method = (Method) method_idx;
					request.url = parts[1];
					request.protocol = parts[2];
				} else {
					int sep=s.find(":");
					if (sep<0)
						sep=s.length();
					String field_name = s.substr(0,sep).strip_edges().to_lower();
					String field_value = s.substr(sep+1,s.length()).strip_edges();

					if (field_name.empty() || field_value.empty())
						continue;

					if (field_name == "content-length") {
						request.body_size = field_value.to_int();
					}

					request.header.insert(field_name, field_value);
				}
			}

			switch (request.method) {
				case METHOD_POST: {

					if (request.body_size == 0) {
						// No content... ignore request
						continue;
					}

					// Read and parse body as json
					Dictionary data;
					Error parse_err = data.parse_json(request.read_utf8_body());

					if (parse_err != OK) {
						request.response.status = "400 Bad Request";
						request.response.set_header("Accept", "application/json");
						request.response.set_header("Accept-Charset", "utf-8");
						request.send_response();
						break;
					}

					// Obtain suggestions based on the request
					String hint;
					Vector<String> suggestions;
					bool valid = cd->ccs->get_service()->obtain_suggestions(data, suggestions, hint);

					if (!valid) {
						request.response.status = "404 Not Found";
						request.send_response();
						break;
					}

					data["hint"]=hint.replace(String::chr(0xFFFF), "\n");
					data["suggestions"]=suggestions;
					data.erase("text");

					// Done! Deliver <3
					request.response.status = "200 OK";
					request.response.set_header("Content-Type", "application/json; charset=UTF-8");
					request.send_response(data.to_json());

				} break;
				default: {

					request.response.status = "405 Method Not Allowed";
					request.response.set_header("Allow", "POST");
					request.send_response();
				} break;
			}

			CLOSE_CLIENT_COND(request.header["connection"] != "keep-alive", cd);
		}
	}

	_close_client(cd);
}

void CodeCompletionServer::_thread_start(void *s) {

	CodeCompletionServer *self = (CodeCompletionServer*)s;

	while(!self->quit) {

		if (self->cmd == CMD_ACTIVATE) {

			int port_max = self->port + 10;

			while (self->port < port_max) {

				if (self->server->listen(self->port)==OK) {
					self->active = true;
					self->cmd = CMD_NONE;
					self->_add_to_servers_list();

					break;
				}

				self->port++;
			}

			if (!self->active)
				self->quit = true;

		} else if (self->cmd == CMD_STOP) {
			self->server->stop();
			self->active = false;
			self->cmd = CMD_NONE;
		}

		if (self->active && self->server->is_connection_available()) {
			ClientData *cd = memnew( ClientData );
			cd->connection=self->server->take_connection();
			cd->ccs = self;
			cd->quit = false;
			cd->thread = Thread::create(_subthread_start, cd);
		}

		self->wait_mutex->lock();
		while (self->to_wait.size()) {
			Thread *w = self->to_wait.front()->get();
			self->to_wait.erase(w);
			self->wait_mutex->unlock();
			Thread::wait_to_finish(w);
			memdelete(w);
			self->wait_mutex->lock();
		}
		self->wait_mutex->unlock();

		OS::get_singleton()->delay_usec(100000);
	}
}

void CodeCompletionServer::start(CodeCompletionService *r_service) {
	stop();
	port = 6070;
	service = r_service;
	cmd = CMD_ACTIVATE;
}

bool CodeCompletionServer::is_active() const {
	return active;
}

void CodeCompletionServer::stop() {
	cmd = CMD_STOP;
}

void CodeCompletionServer::_add_to_servers_list() {

	static String serversPath = EditorSettings::get_singleton()->get_settings_path() + "/.autocomplete-servers.json";

	Dictionary serversList;

	FileAccess *f_read=FileAccess::open(serversPath,FileAccess::READ);
	if (f_read) {
		String text;
		String l = f_read->get_line();
		while(!f_read->eof_reached()) {
			text+=l+"\n";
			l = f_read->get_line();
		}
		text+=l;

		serversList.parse_json(text);
		f_read->close();
		memdelete(f_read);
	}

	FileAccess *f_write = FileAccess::open(serversPath,FileAccess::WRITE);
	if (f_write) {
		String serverPort = String::num(port);

		if (!serversList.empty()) {
			Array keys = serversList.keys();
			for (int i = 0; i < keys.size(); i++) {
				String md5path = keys[i];
				if (serversList[md5path] == serverPort) {
					serversList.erase(md5path);
					break;
				}
			}
		}

		serversList[Globals::get_singleton()->get_resource_path().md5_text()] = serverPort;

		f_write->store_string(serversList.to_json());
		f_write->close();
		memdelete(f_write);
	}
}

CodeCompletionServer::CodeCompletionServer() {

	server = TCP_Server::create_ref();
	wait_mutex = Mutex::create();
	quit = false;
	active = false;
	cmd = CMD_NONE;
	thread = Thread::create(_thread_start, this);
}

CodeCompletionServer::~CodeCompletionServer() {

	quit = true;
	Thread::wait_to_finish(thread);
	memdelete(thread);
	memdelete(wait_mutex);
}
