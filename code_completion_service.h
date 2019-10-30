/* code_completion_service.h */
#ifndef CODE_COMPLETION_SERVICE_H
#define CODE_COMPLETION_SERVICE_H

#include "scene/main/node.h"

class CodeCompletionService : public Node {

	GDCLASS(CodeCompletionService, Node);

public:
	struct Request {
		String script_path;
		bool has_script_text;
		String script_text;
		int row;
		int column;

		Request() {
			has_script_text = false;
			row = 0;
			column = 0;
		}
	};

	struct Result {
		bool valid;
		String prefix;
		String hint;
		Vector<String> suggestions;

		Result(bool p_valid=false) {
			valid = p_valid;
		}
	};

	Result obtain_suggestions(const Request& p_request);

	CodeCompletionService();
	~CodeCompletionService();

private:
	Set<String> completion_prefixes;
	List<StringName> type_keywords;
	List<String> language_keywords;

	Node* _find_node_for_script(Node* p_base, Node* p_current, const Ref<Script>& p_script);
	String _get_text_for_completion(const Request& p_request, String& r_text);
	String _filter_completion_candidates(int p_col, const String& p_line, const List<String>& p_options, Vector<String> &r_suggestions);
};

#endif // CODE_COMPLETION_SERVICE_H
