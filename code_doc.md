TODO
- query filtering and organization for inputs like rooms, buildings etc
    - SecureHTTPSecureHandler - do_GET(self)

#### junebug.py

- \_async\_riase(tid, exectype)

KillableThread(threading.Thread)
- \_\_init\_\_(self, \*args, \*\*kwargs)
- stop(self)
- stopped(self)
- \_get\_tid(self)

Runner(KillableThread)
- \_\_init\_\_(self, \*args, \*\*kwargs)
- run(self)

DataHandler(KillableThread)
- \_\_init\_\_(self, \*args, \*\*kwargs)
- get_data(self)
- run(self)

ServerThread(KillableThread)
- \_\_init\_\_(self, \*args, \*\*kwargs)
- run(self)

SecureHTTPServer(HTTPServer)
- \_\_init\_\_

SecureHTTPRequestHandler (SiimpleHTTPRequestHandler):
- setup(self)
- get_args(self, api_path)
- do_GET(self)
- do_POST(self)

---

### https_server.py & https_socketserver.py
Lifted from the https library but very minor changes were made.
https changes, a shutdown, parameter
socketserver, import httpsserver change

---

### pull_lldp.c
TODO
- make it read and writeable at the same time (not sure if best).
- 