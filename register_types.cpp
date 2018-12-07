/* register_types.cpp */
#include "register_types.h"

#ifdef TOOLS_ENABLED
#define AUTOCOMPLETE_SERVICE
#endif

#ifdef AUTOCOMPLETE_SERVICE
#include "completion_service_plugin.h"
#endif

void register_autocomplete_service_types() {
#ifdef AUTOCOMPLETE_SERVICE
	EditorPlugins::add_by_type<CompletionServicePlugin>();
#endif
}

void unregister_autocomplete_service_types() {

}
