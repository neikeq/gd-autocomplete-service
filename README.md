
# Godot Auto-complete Service
Godot module that listens for http requests and returns autocomplete suggestions. Under test.

### Use it from your provider

Requesting for completion suggestions from your provider is quite simple.
The server listens for HTTP Request on http://localhost:6070 (**this address or port may change in the future**).

The following is a request example. Currently, only _POST_ method is allowed. The body must be a JSON with the completion request information.

```
POST http://localhost:6070 HTTP/1.1
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

The important part here are the following field:

- **hint** Sometimes, a hint may be returned. It contains a brief information about the current function, and it may be used for other stuff too (not sure).
- **suggestions** The resulted list of completion suggestions.
- **prefix** The prefix string to be replaced with the user's chosen suggestion.

### Providers

- [atom-autocomplete-gdscript](https://github.com/neikeq/atom-autocomplete-gdscript)
