
# Godot Auto-complete Service
 Godot module that provides autocomplete suggestions via HTTP requests
### Installation Instruction
clone the repo into the modules directory in the godot engine source code like so:
godot/modules/gd_autocomplete_service

compile the source code according to the instructions here:
https://docs.godotengine.org/en/latest/development/compiling/index.html

### Providers

- [atom-autocomplete-gdscript](https://github.com/neikeq/atom-autocomplete-gdscript)

### Usage from providers

The server listens for HTTP Requests on http://localhost:port. The port varies depending of how many editors are providing suggestions at the same time.

##### Getting the project path

First you need to know the path of the project that owns the script. From the script directory, search backwards in the parent directories until you find the one that contains the file `project.godot`. This is the project path.

Example from the [autocomplete-gdscript](https://github.com/neikeq/atom-autocomplete-gdscript/blob/master/lib/provider.coffee#L58-L60) atom package:

``` CoffeeScript
currentDir = new File(filePath).getParent()
while not currentDir.getFile("project.godot").existsSync()
    currentDir = currentDir.getParent()
```

##### Getting the correct port

Now you need to know in which port the server that provides suggestions for this project is listening. The list of servers is stored in the following file:

- Unix: `$HOME/.config/godot/.autocomplete-servers.json`
- Windows: `$APPDATA\Godot\.autocomplete-servers.json`

This is the JSON content:

``` json
{ "Project path MD5": "port" }
```

The keys are MD5 hashes of the project path.

##### When to read the servers list

There may be multiple situations where you will need to read the servers list file to get the server port. These are three example situations:

- The first time the provider gets a suggestions request for a project.
- When the server that provides suggestions for your project is no longer listening on the port currently known (note that there may not be any server providing suggestions for that project).
- When the server returns a response with status code 404. This means the server listening on the port currently known is providing suggestions for a different project.

There may be other situations. You may want to ensure you have the correct port when a request results in an error not specified above. It depends of how the provider is implemented.

#### Request example

The server only accepts requests with method _POST_.

##### Header

```
POST http://localhost:port HTTP/1.1
Accept: application/json
Connection: keep-alive
Content-Type: application/json; charset=UTF-8
Content-Length: 217
```

##### Body

The body of the request must be an UTF-8 encoded JSON with the code completion request information (read below).

The following is the JSON structure:

``` json
{
    "path": "/absolute/system/path/to/script.gd",
    "text": "The script content.",
    "cursor": {
        "row": 0,
        "column": 0
    },
    "meta": "Ignored by the server, but returned in the response."
}
```

The text field is optional, but it's recommended not to skip it. It must contain the script content. If skipped, the server will load the last saved or cached version of the file, which could be outdated compared to the one being edited.

The meta field is also optional. It can contain any information you wish to receive back with the response.

#### Response example

If the request is valid and everything went well, you should receive a response like this:

```
HTTP/1.1 200 OK
Server: Godot Auto-complete service
Connection: keep-alive
Content-Type: application/json; charset=UTF-8
Content-Length: 390
```

``` json
{
    "path": "/absolute/system/path/to/script.gd",
    "cursor": {
        "row": 0,
        "column": 0
    },
    "meta": "Ignored by the server, but returned in the response.",
    "hint": "Node find_node( String mask, bool recursive=true, bool owned=true )",
    "suggestions": [
        "get_child(",
        "get_child_count()",
        "get_children()"
    ],
    "prefix": "get_ch"
}
```

Important fields:

- **hint** When typing method parameters, the response returns a hint with information about the function return type and its parameters. The hint encloses the current parameter with line breaks. Example: `Node get_node( \nNodePath path\n )`. You can replace the line breaks to fit your editor style. For example, with HTML bold tags: `Node get_node( <b>NodePath path</b> )`.
- **suggestions** The resulted list of auto-complete suggestions.
- **prefix** The prefix to be replaced with the user's chosen suggestion.
