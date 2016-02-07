/* code_completion_server.cpp */
#include "code_completion_server.h"
#include "os/file_access.h"
#include "tools/editor/editor_settings.h"
#include "globals.h"

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

		if (err!=OK) {
			_close_client(cd);
			ERR_FAIL_COND(err!=OK);
		}

		request_str.push_back(byte);

		int rs = request_str.size();
		if ((rs>=2 && request_str[rs-2]=='\n' && request_str[rs-1]=='\n') ||
			(rs>=4 && request_str[rs-4]=='\r' && request_str[rs-3]=='\n' && rs>=4 && request_str[rs-2]=='\r' && request_str[rs-1]=='\n')) {

			// end of request, parse
			request_str.push_back(0);
			String request;
			request.parse_utf8((const char*)request_str.ptr());
			Vector<String> requests = request.split("\n");
			int body_size=0;
			Vector<String> request_headers;
			Method method=METHOD_GET;

			request_str.clear();

			bool keep_alive=false;

			for (int i = 0; i < requests.size(); i++) {
				String s = requests[i].strip_edges();
				if (s.length() == 0)
					continue;

				if (i == 0) {
					// parse command, path and version
					String method_str = s.substr(0, s.find(" "));

					int method_idx = -1;
					for (int j = 0; j < METHOD_MAX; j++) {
						if (_methods[j] == method_str) {
							method_idx = j;
							break;
						}
					}

					if (method_idx < 0) {
						_close_client(cd);
						return;
					}

					method = (Method)method_idx;
				} else {
					if (s.to_lower().begins_with("content-length:")) {
						body_size=s.substr(s.find(":") + 1, s.length()).strip_edges().to_int();
					} else if (s.to_lower().begins_with("connection:")) {
						keep_alive=s.substr(s.find(":") + 1, s.length()).strip_edges()=="keep-alive";
					}

					request_headers.push_back(s);
				}
			}

			switch (method) {
				case METHOD_POST: {

					if (body_size == 0) {
						// no content... continue with next request
						continue;
					}

					// read body
					String json_body = cd->connection->get_utf8_string(body_size);

					// parse body
					Dictionary data;
					Error parse_err = data.parse_json(json_body);

					if (parse_err!=OK) {

						String resp_status = "400 Bad Request";
						String resp_headers = "Accept: application/json\r\n";
						resp_headers += "Accept-Charset: utf-8\r\n";
						_write_response(cd, resp_status, resp_headers, String(), !keep_alive);
						break;
					}

					// obtain suggestions based on the request
					String hint;
					Vector<String> suggestions;
					bool valid = cd->ccs->get_service()->obtain_suggestions(data, suggestions, hint);

					if (!valid) {
						String resp_status = "404 Not Found";
						_write_response(cd, resp_status, "", String(), !keep_alive);
						break;
					}

					data["hint"]=hint.replace(String::chr(0xFFFF), "\n");
					data["suggestions"]=suggestions;
					data.erase("text");

					// done! deliver <3
					String resp_status = "200 OK";
					String resp_headers = "Content-Type: application/json; charset=UTF-8\r\n";
					String resp_body = data.to_json();
					_write_response(cd, resp_status, resp_headers, resp_body, !keep_alive);

				} break;
				default: {

					String resp_status = "405 Method Not Allowed";
					String resp_headers = "Allow: POST\r\n";
					_write_response(cd, resp_status, resp_headers, String(), !keep_alive);
				} break;
			}

			if (!keep_alive) {
				// fall to close connection
				break;
			}
		}
	}

	_close_client(cd);
}

void CodeCompletionServer::_write_response(ClientData *cd, const String& p_status, const String& p_headers, const String& p_body, bool p_close) {

	String response = "HTTP/1.1 " + p_status + "\r\n";
	response += "Server: Godot Auto-complete Service\r\n";
	response += p_headers;
	response += "Connection: " + String(p_close?"close":"keep-alive") + "\r\n";

	if (!p_body.empty())
		response += "Content-Length: " + itos(p_body.length()) + "\r\n";

	response += "\r\n";
	response += p_body;

	cd->connection->put_utf8_string(response);
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

					String serversPath = EditorSettings::get_singleton()->get_settings_path() + "/.autocomplete-servers.json";
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
						String serverPort = String::num(self->port);

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
