/* completion_service_plugin.h */
#ifndef COMPLETION_SERVICE_PLUGIN_H
#define COMPLETION_SERVICE_PLUGIN_H

#include "../../editor/editor_plugin.h"
#include "code_completion_server.h"

class CompletionServicePlugin : public EditorPlugin {

	GDCLASS(CompletionServicePlugin, EditorPlugin);

	CodeCompletionServer *server;
	CodeCompletionService *service;
	EditorNode *editor;

protected:
	void _notification(int p_what);

public:
	CompletionServicePlugin(EditorNode *p_node);
	~CompletionServicePlugin();
};

#endif // COMPLETION_SERVICE_PLUGIN_H
