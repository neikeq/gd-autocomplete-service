/* register_types.cpp */
#include "register_types.h"

#ifdef TOOLS_ENABLED
#include "completion_service_plugin.h"
#endif

void register_autocomplete_service_types() {
#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<CompletionServicePlugin>();
#endif
}

void unregister_autocomplete_service_types() {

}
