
# Godot Auto-complete Service
Godot module that listens for HTTP Requests and returns auto-complete suggestions. Under test.

### Use it from your provider

Requesting for completion suggestions from your provider is quite simple.
The server listens for HTTP Requests on http://localhost:port. The port varies depending of how many servers (editors) are listening at the same time.

##### Getting the correct port

The list of servers is stored in the following file:

```
$HOME/.godot/.autocomplete-servers.json
$APPDATA\Godot\.autocomplete-servers.json
```

Content:

``` json
{ "md5 of the project path": "port" }
```

The key is the md5 hash of the project path. To know the project path for a file, you must search backwards in the parent directories of your script until you find the directory that contains the file `engine.cfg`.

With this information you can retrieve the port for the server that provides code completion suggestions for your project.

##### When to retrieve the port

This depends of how the provider is implemented. There are 3 example situations where you may need to read the servers list to know your port.

- When you don't know the port for a project (either because this is the first request for that project, or because no server is providing suggestions for that project at that moment).
- When the server returns a response with status code 404. This means the server no longer provides suggestions for this project.
- Depending of the situation, it may be needed when a request results in an error.

##### Request example

Currently, only _POST_ method is allowed. The body must be a JSON with the completion request information.

```
POST http://localhost:port HTTP/1.1
Accept: application/json
Connection: keep-alive
Content-Type: application/json; charset=UTF-8
Content-Length: X

{} ; body. see below
```

The following is the JSON structure.

The text field is optional, but it's recommended not to skip it. It must contain the script content. If skipped, the server will load the last saved version of the file, which could be outdated compared to the one being edited.

The meta field is also optional. It can contain any information you wish to receive back with the response.

``` json
{
    "path": "/absolute/system/path/to/script.gd",
    "text": "The script content.",
    "cursor": {
        "row": 0,
        "column": 0
    },
    "meta": "Ignored by the service. Returned in the response."
}
```

##### Response example

If everything went well, you should receive a response like this:

```
HTTP/1.1 200 OK
Server: Godot Auto-complete service
Connection: keep-alive
Content-Type: application/json; charset=UTF-8
Content-Length: X
```

``` json
{
    "path": "/absolute/system/path/to/script.gd",
    "cursor": {
        "row": 0,
        "column": 0
    },
    "meta": "Ignored by the service. Returned in the response.",
    "hint": "Node find_node( String mask, bool recursive=true, bool owned=true )",
    "suggestions": [
        "get_child(",
        "get_child_count()",
        "get_children()"
    ],
    "prefix": "get_ch"
}
```

The important parts are the following field:

- **hint** Sometimes, a hint may be returned. It contains a brief information about the current function, and it may be used for other stuff too (not sure).
- **suggestions** The resulted list of completion suggestions.
- **prefix** The prefix string to be replaced with the user's chosen suggestion.

### Providers

- [atom-autocomplete-gdscript](https://github.com/neikeq/atom-autocomplete-gdscript)
