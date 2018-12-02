/* code_completion_service.cpp */

#include "code_completion_service.h"
//#include "io/resource_loader.h"
//#include "globals.h"
#ifdef GDSCRIPT_ENABLED
#include "../gdscript/gdscript.h"
#endif

static bool _is_symbol(CharType c) {

	return c!='_' && ((c>='!' && c<='/') || (c>=':' && c<='@') || (c>='[' && c<='`') || (c>='{' && c<='~') || c=='\t');
}

static bool _is_completable(CharType c) {

	return !_is_symbol(c) || c=='"' || c=='\'';
}

CodeCompletionService::Result CodeCompletionService::obtain_suggestions(const Request& p_request) {

    #ifdef GDSCRIPT_ENABLED
	Result result(true);

	String path = ProjectSettings::get_singleton()->localize_path(p_request.script_path);

	if (path == "res://" || !path.begins_with("res://"))
		return Result();

	Ref<Script> script = ResourceLoader::load(path);

	if (!script.is_valid() || !Object::cast_to<GDScript>(*script))
		return Result();

	String script_text = p_request.script_text;

	if (script_text.empty()) {
		if (p_request.has_script_text)
			return Result();

		ERR_FAIL_COND_V(!script->has_source_code(), Result());
		script_text = script->get_source_code();
	}

	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		base = _find_node_for_script(base,base,script);
	}

	String current_line = _get_text_for_completion(p_request, script_text);

	List<String> options;
    bool force = false;
	script->get_language()->complete_code(script_text, script->get_path().get_base_dir(), base, &options, force, result.hint);

	if (options.size()) {
		result.prefix = _filter_completion_candidates(p_request.column, current_line, options, result.suggestions);
	}

	return result;
	#else
	return Result();
    #endif
}

String CodeCompletionService::_get_text_for_completion(const Request& p_request, String& r_text) {

	Vector<String> substrings = r_text.replace("\r","").split("\n");
	r_text.clear();

	int len = substrings.size();

	for (int i=0;i<len;i++) {
		if (i==p_request.row) {
			r_text+=substrings[i].substr(0,p_request.column);
			r_text+=String::chr(0xFFFF); //not unicode, represents the cursor
			r_text+=substrings[i].substr(p_request.column,substrings[i].size());
		} else {
			r_text+=substrings[i];
		}

		if (i!=len-1)
			r_text+="\n";
	}

	return substrings[p_request.row];
}

Node* CodeCompletionService::_find_node_for_script(Node* p_base, Node* p_current, const Ref<Script>& p_script) {

	if (p_current->get_owner()!=p_base && p_base!=p_current)
		return NULL;
	Ref<Script> c = p_current->get_script();
	if (c==p_script)
		return p_current;
	for(int i=0;i<p_current->get_child_count();i++) {
		Node *found = _find_node_for_script(p_base,p_current->get_child(i),p_script);
		if (found)
			return found;
	}

	return NULL;
}

String CodeCompletionService::_filter_completion_candidates(int p_col, const String& p_line, const List<String>& p_options, Vector<String> &r_suggestions) {

	int cofs = CLAMP(p_col,0,p_line.length());

	//look for keywords first

	bool inquote=false;
	int first_quote=-1;

	int c=cofs-1;
	while(c>=0) {
		if (p_line[c]=='"' || p_line[c]=='\'') {
			inquote=!inquote;
			if (first_quote==-1)
				first_quote=c;
		}
		c--;
	}

	String prefix;

	bool pre_keyword=false;
	bool cancel=false;

	if (!inquote && first_quote==cofs-1) {

		cancel=true;

	} if (inquote && first_quote!=-1) {

		prefix=p_line.substr(first_quote,cofs-first_quote);

	} else if (cofs>0 && p_line[cofs-1]==' ') {
		int kofs=cofs-1;
		String kw;
		while (kofs>=0 && p_line[kofs]==' ')
			kofs--;

		while(kofs>=0 && p_line[kofs]>32 && _is_completable(p_line[kofs])) {
			kw=String::chr(p_line[kofs])+kw;
			kofs--;
		}

		pre_keyword=type_keywords.find(kw) || language_keywords.find(kw);

	} else {


		while(cofs>0 && p_line[cofs-1]>32 && _is_completable(p_line[cofs-1])) {
			prefix=String::chr(p_line[cofs-1])+prefix;
			if (p_line[cofs-1]=='\'' || p_line[cofs-1]=='"')
				break;

			cofs--;
		}
	}

	if (cancel || (!pre_keyword && prefix=="" && (cofs==0 || !completion_prefixes.has(String::chr(p_line[cofs-1])))))
		return String();

	for (const List<String>::Element *E=p_options.front();E;E=E->next()) {
		if (E->get().begins_with(prefix)) {
			r_suggestions.push_back(E->get());
		}
	}

	return prefix;
}

CodeCompletionService::CodeCompletionService() {

	completion_prefixes.insert(".");
	completion_prefixes.insert(",");
	completion_prefixes.insert("(");

	type_keywords.push_back("Vector2");
	type_keywords.push_back("Vector3");
	type_keywords.push_back("Plane");
	type_keywords.push_back("Quat");
	type_keywords.push_back("AABB");
	type_keywords.push_back("Matrix3");
	type_keywords.push_back("Transform");
	type_keywords.push_back("Color");
	type_keywords.push_back("Image");
	type_keywords.push_back("InputEvent");

    // TODO: not sure what this is supposed to do
    //	ObjectTypeDB::get_type_list(&type_keywords);

	#ifdef GDSCRIPT_ENABLED
	GDScriptLanguage::get_singleton()->get_reserved_words(&language_keywords);
	#endif
}

CodeCompletionService::~CodeCompletionService() {
}
