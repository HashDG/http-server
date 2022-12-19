# C HTTP Server

Here is an HTTP Server written in C. It support browsing through HTML files and also handlers. The browsing works with an abstract hashtable, key is the URL and value is a structure containing a file or a handler. The handler is a function pointer returning a dynamically generated HTML file.

It all began from a Youtube video shared by a friend ([here](https://www.youtube.com/watch?v=cEH_ipqHbUw)). I wanted to do mine, and push it further :)

## Execution

```
git clone https://github.com/HashDG/http-server.git
cd http-server
make
./http-server
```

Start Firefox and visit `127.0.0.1:1080`

## Adding content

By default, HTML files have to be put in the `http_ressources` folder, you can put all the files you need as long as it's indexed in the MIME database. 

Furthermore, if you want to add handlers :
- write your handler in `session.c` as a function. Warning it has to follow this signature `char * handler(char*,char**)`
- add your handler in the `load_handlers` function following the example of `simple_handler`

## TODO

- [ ] Arguments for handlers
- [ ] More methods for handlers
- [X] CSS support
- [ ] Configuration file
