#!/bin/env/var python3

interfaceDict = {}

has_confirmed = False
print("Initialized.")

while not(has_confirmed):
    confirm = input('Type "enable" to continue: ')
    if (confirm == "enable"):
        has_confirmed = True

run = True
while (run):
    command = input("> ")
    if command == "show lldp interface all":
        print("""
Local Interface    Parent Interface    Chassis Id          Port info          System Name
ge-0/0/3.0         -                   12:34:56:78:9A:BC   151                ITC-0106-PROC1
ge-0/0/4.0         -                   12:34:56:78:9A:BC   151                ITC-0106-TP1
ge-0/0/6.0         -                   12:34:56:78:9A:BC   151                ITC-0106-DISP1
        """)
    elif command.startswith("show lldp interface"):
        comList = command.split(" ")
        if len(comList) == 5:
            if comList[4] in interfaceDict.keys():
                try:
                    print(interfaceDict.get(comList[3]))
                except:
                    print("[-] I_ERR: Interface unknown.")
            else:
                print("[-] I_ERR: Interface unknown.")
        else:
            print("[-] A_ERR: Improper number of arguments.")
    elif command == "quit":
        print("Goodbye.")
        run = False
    else:
        print("[-] Error: Unknown command.")

print("")