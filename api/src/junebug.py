import argparse
import os
import threading
import inspect
import ctypes
import queue
import socket
from https_socketserver import BaseServer
from https_server import HTTPServer, SimpleHTTPRequestHandler
import csv
from io import BytesIO
from OpenSSL import SSL

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
            if self.counter % 2:
                switch_type = "netgear"
            else:
                switch_type = "juniper"
            
            os.system(f"{filepath}../bin/pull_lldp.out {switch_type} test")

            with open('records_out.csv', 'r') as csvfile:
                csv_reader = csv.reader(csvfile)
                for row in csv_reader:
                    self.collector.put((row[1], row[3]))

            self.counter += 1

class DataHandler(KillableThread):
    def __init__(self, *args, **kwargs):
        super(DataHandler, self).__init__(*args, *kwargs)
        self.records = {}

    def get_data(self):
        return self.records
    
    def run(self):
        while True:
            if not runnerThread.collector.empty():
                record = runnerThread.collector.get()
                if record[0] in self.records.keys():
                    uptime = self.records.get(record[0])[1]
                    self.records.update({record[0]: (record[1], uptime+1)})
                else:
                    self.records.update({record[0]: (record[1], 1)})
        
class ServerThread(KillableThread):
    def __init__(self, *args, **kwargs):
        super(ServerThread, self).__init__(*args, **kwargs)
    
    def run(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("port", type=int, nargs="?", const=1, default=8888)
        args = parser.parse_args()
        sport = args.port
        sip = '127.0.0.1'
        while True:
            try:
                httpd = SecureHTTPServer((sip, sport), SecureHTTPRequestHandler)
                break
            except Exception as e:
                print(f"[!] Caught error: {e}")
                sport += 1
                pass
        address = httpd.socket.getsockname()
        print(f"-- [+] Junebug mounted at {address[0]}:{address[1]} --")
        httpd.serve_forever()

class SecureHTTPServer(HTTPServer):
    def __init__(self, server_location, handler):
        BaseServer.__init__(self, server_location, handler)
        ctx = SSL.Context(SSL.TLS_SERVER_METHOD)
        key_pem = f"{filepath}../keys/key.pem"
        cert_pem = f"{filepath}../keys/cert.pem"
        ctx.use_privatekey_file(key_pem)
        ctx.use_certificate_file(cert_pem)
        self.socket = SSL.Connection(ctx, socket.socket(self.address_family, self.socket_type))
        self.server_bind()
        self.server_activate()

class SecureHTTPRequestHandler(SimpleHTTPRequestHandler):
    def setup(self):
        self.connection = self.request
        self.rfile = socket.SocketIO(self.request, "rb")
        self.wfile = socket.SocketIO(self.request, "wb")

    def get_args(self, api_path):
        argsDict = {}
        try:
            args = api_path.split("?")[1]
        except Exception as e:
            return {"error": "[+] U_ERR: No parameters provided."}
        
        try:
            for arg in args.split("&"):
                splitArg = arg.split("=")
                argsDict.update({splitArg[0]: splitArg[1]})
        except Exception as e:
            return {"error": "[+] A_ERR: Parameter provided without argument."}

        return argsDict
                
        
    def do_GET(self):
        path = self.path
        resp = "{ \"alert\": \"[-] U_ERR: Unknown URL called.\" }"
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        kill = False

        if path == "/":
            resp = open("help.json", "r").read()
        elif path.startswith("/api"):
            data = handlerThread.get_data()
            argDict = self.get_args(path)
            if "error" in argDict.keys():
                resp = "{ \"error\": \"" + argDict["error"] + "\" }"
            else:
                resp = "{ \"rooms\": ["
                for key in data.keys():
                    room = data.get(key)
                    resp += " {\"name\": \"" + key + "\", \"ip\": \"" + room[0] + "\", \"uptime\": " + str(room[1]) + "}, "
                resp = resp[:-2]
                resp += "] }"
        elif path.startswith("/help"):
            resp = open("help.json", "r").read()
        elif path.startswith("/kill"):
            resp = "{ \"alert\": \"[+] KILL call made... Shutting down server.\" }"
            kill = True

        self.wfile.write(bytes(resp, "ascii"))
        if kill:
            runnerThread.raise_exc(SystemExit)
            handlerThread.raise_exc(SystemExit)
            reqThread.raise_exc(SystemExit)
        

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
        
filepath = __file__.removesuffix("junebug.py")
os.system(f"gcc {filepath}pull_lldp.c -o {filepath}../bin/pull_lldp.out")
runnerThread = Runner()
runnerThread.start()
handlerThread = DataHandler()
handlerThread.start()
reqThread = ServerThread()
reqThread.start()