/* code_completion_service.cpp */

#include "code_completion_service.h"
#include "io/resource_loader.h"
#include "globals.h"

static bool _is_symbol(CharType c) {

	return c!='_' && ((c>='!' && c<='/') || (c>=':' && c<='@') || (c>='[' && c<='`') || (c>='{' && c<='~') || c=='\t');
}

static bool _is_completable(CharType c) {

	return !_is_symbol(c) || c=='"' || c=='\'';
}

void CodeCompletionService::obtain_suggestions(Dictionary &r_request, Vector<String> &r_suggestions, String& hint) {

	String path = Globals::get_singleton()->localize_path(r_request["path"]);
	String code = r_request["text"];
	Dictionary cursor = r_request["cursor"];
	int row = cursor["row"];
	int col = cursor["column"];

	Ref<Script> script = ResourceLoader::load(path);

	if (!script.is_valid())
		return;

	if (code.empty()) {
		ERR_FAIL_COND(!script->has_source_code());
		code = script->get_source_code();
	}

	Vector<String> code_lines;
	_get_text_for_completion(code, code_lines, row, col);

	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		base = _find_node_for_script(base,base,script);
	}

	List<String> options;
	script->get_language()->complete_code(code, script->get_path().get_base_dir(), base, &options, hint);

	if (options.size() > 0) {

		List<String> language_keywords;
		script->get_language()->get_reserved_words(&language_keywords);

		r_request["prefix"] = _filter_completion_candidates(col, code_lines[row], language_keywords, options, r_suggestions);

	} else {
		r_request["prefix"] = String();
	}
}

void CodeCompletionService::_get_text_for_completion(String& p_text, Vector<String>& substrings, int p_row, int p_col) {

	substrings = p_text.replace("\r","").split("\n");
	p_text.clear();

	int len = substrings.size();
	for (int i=0;i<len;i++) {

		if (i==p_row) {
			p_text+=substrings[i].substr(0,p_col);
			p_text+=String::chr(0xFFFF); //not unicode, represents the cursor
			p_text+=substrings[i].substr(p_col,substrings[i].size());
		} else {
			p_text+=substrings[i];
		}

		if (i!=len-1)
			p_text+="\n";
	}
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

String CodeCompletionService::_filter_completion_candidates(int p_col, const String& p_line, List<String>& p_lang_keywords, const List<String>& p_options, Vector<String> &r_suggestions) {

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

		pre_keyword=keywords.find(kw) || p_lang_keywords.find(kw);

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

	keywords.push_back("Vector2");
	keywords.push_back("Vector3");
	keywords.push_back("Plane");
	keywords.push_back("Quat");
	keywords.push_back("AABB");
	keywords.push_back("Matrix3");
	keywords.push_back("Transform");
	keywords.push_back("Color");
	keywords.push_back("Image");
	keywords.push_back("InputEvent");

	ObjectTypeDB::get_type_list(&keywords);
}

CodeCompletionService::~CodeCompletionService() {
}
