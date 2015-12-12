
# Godot Auto-complete Service
Godot module that listens for http requests and returns autocomplete suggestions. Under test.

### Request Example

##### Header

```
POST http://localhost:6070 HTTP/1.1
Accept: application/json
Connection: keep-alive
Content-Type: application/json; charset=UTF-8
Content-Length: X
```

##### Body

``` json
{
    "path": "/absolute/system/path/to/script.gd",
    "text": "Script content. If empty, the service will load the last saved content for the script.",
    "cursor": {
        "row": 0,
        "column": 0
    },
    "meta": "Ignored by the service. Returned in the response."
}
```

### Response Example

##### Header

```
HTTP/1.1 200 OK
Server: Godot Auto-complete service
Connection: keep-alive
Content-Type: application/json; charset=UTF-8
Content-Length: X
```

##### Body

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
