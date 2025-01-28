import time
import threading
import inspect
import ctypes
import queue
from http.server import HTTPServer, BaseHTTPRequestHandler
import re
from io import BytesIO
import ssl

def _async_raise(tid, exctype):
    if not inspect.isclass(exctype):
        raise TypeError("[-] T_ERR: Only types can be raised (not instances).")
    res = ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(tid),
                                                     ctypes.py_object(exctype))
    
    if res == 0:
        raise ValueError("[-] V_ERR: Invalid thread id.")
    elif res != 1:
        ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(tid),
                                                   None)
        raise SystemError("[-] S_ERR: PyThreadState_SetAsyncExc failed.")
    
class KillableThread(threading.Thread):
    def __init__(self, *args, **kwargs):
        super(KillableThread, self).__init__(*args, **kwargs)
        self._stop_event = threading.Event()
    
    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()

    def _get_tid(self):
        if not self.is_alive():
            raise threading.ThreadError("[-] T_ERR: Not an actve thread.")
        
        if hasattr(self, "_tread_id"):
            return self._thread_id
        
        for tid, tobj in threading._active.items():
            if tobj is self:
                self._thread_id = tid
                return tid
            
        raise AssertionError("[-] A_ERR: Could not determine tid.")
    
    def raise_exc(self, exctype):
        _async_raise(self._get_tid(), exctype)

class Runner(KillableThread):
    def __init__(self, *args, **kwargs):
        super(Runner, self).__init__(*args, **kwargs)
        self.running = True
        self.collector = queue.Queue()
        self.counter = 0

    def run(self):
        while self.running:
            self.collector.put(self.counter)
            self.counter += 1
            time.sleep(1)

class DataHandler(KillableThread):
    def __init__(self, child, *args, **kwargs):
        super(DataHandler, self).__init__(*args, *kwargs)
        self.child = child
        self.child_tid = self.child._get_tid()
        self.counter = 0
    
    def run(self):
        while True:
            if not self.child.collector.empty():
                self.counter = self.child.collector.get()
        
class ServerThread(KillableThread):
    def __init__(self, datasrc, *args, **kwargs):
        super(ServerThread, self).__init__(*args, **kwargs)
        self.datasrc = datasrc
        self.data = 0
        self.filepath_base = __file__.removesuffix("test.py")

    def get_data(self):
        self.data = self.datasrc.counter
        return self.data
    
    def run(self):
        sip = '127.0.0.1'
        sport = 7878
        while True:
            try:
                httpd = HTTPServer((sip, sport), HTTPRequestHandler)
                # sock_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
                # sock_context.load_cert_chain(certfile=f"{self.filepath_base}cert.pem", keyfile=f"{self.filepath_base}key.pem")
                # httpd.socket = sock_context.wrap_socket(httpd.socket, server_side=True)
                break
            except Exception as e:
                print(f"[!] Caught error: {e}")
                sport += 1
                pass
        print(f"-- [+] Junebug mounted at {sip}:{sport} --")
        httpd.serve_forever()

class HTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = self.path
        resp = "[-] U_ERR: Unknown URL."
        self.send_response(200)
        self.end_headers()

        if path == "/":
            resp = "[+] HOME call made."
        elif path.startswith("/api"):
            resp = "[+] API call made."
        elif path.startswith("/config"):
            resp = "[+] CONFIG call made."

        self.wfile.write(bytes(resp, "ascii"))

    def do_POST(self):
        path = self.path
        self.send_response(200)
        self.end_headers()
        resp = BytesIO()
        resp.write(b"Got a POST request.\n")
        resp.write(b"Got: ")
        resp.write(bytes(path, "ascii"))
        resp.write(b'\n')
        self.wfile.write(resp.getvalue())

runnerThread = Runner()
runnerThread.start()
handlerThread = DataHandler(runnerThread)
handlerThread.start()
reqThread = ServerThread(handlerThread)
reqThread.start()