/* code_completion_service.h */
#ifndef CODE_COMPLETION_SERVICE_H
#define CODE_COMPLETION_SERVICE_H

#include "scene/main/node.h"

class CodeCompletionService : public Node {

	OBJ_TYPE(CodeCompletionService, Node);

	Set<String> completion_prefixes;
	List<StringName> keywords;

	Node* _find_node_for_script(Node* p_base, Node*p_current, const Ref<Script>& p_script);
	void _get_text_for_completion(String& p_text, Vector<String>& substrings, int p_row, int p_col);
	String _filter_completion_candidates(int p_col, const String& p_line, List<String>& p_lang_keywords, const List<String>& p_options, Vector<String> &r_suggestions);

public:
	void obtain_suggestions(Dictionary &r_request, Vector<String> &r_suggestions, String& hint);

	CodeCompletionService();
	~CodeCompletionService();
};

#endif // CODE_COMPLETION_SERVICE_H
