/* completion_service_plugin.cpp */

#include "completion_service_plugin.h"

void CompletionServicePlugin::_notification(int p_what) {

	if (p_what == NOTIFICATION_ENTER_TREE) {
		server->start(service);
	}

	if (p_what == NOTIFICATION_EXIT_TREE) {
		server->stop();
	}
}

CompletionServicePlugin::CompletionServicePlugin(EditorNode *p_node) {

	editor = p_node;

	server = memnew( CodeCompletionServer );
	service = memnew( CodeCompletionService );
	add_child(service);
}

CompletionServicePlugin::~CompletionServicePlugin() {

	memdelete( server );
}
