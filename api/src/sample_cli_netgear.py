#!/bin/env/var python3

interfaceDict = {
    "ge-0/0/3.0": '''
Parent Interface   : -
Local Port ID      : 509
Ageout Count       : 0

Neighbour Information:
Chassis type       : Mac address
Chassis ID         : 00:10:7f:8d:91:ed
Port type          : Mac address
Port ID            : 00:10:7f:8d:91:ed
Port description   : eth0
System name        : ITC-0173-TP1

System Description :  Linux 3.10.33 #4 SMP PREEMPT Wed Mar 21 23:16:38 EDT 2018 armv7l

System capabilities
        Supported  : Bridge WLAN Access Point Router Station Only
        Enabled    : Station Only

Management Info
        Type              : IPv4
        Address           : 10.126.4.78
        Port ID           : 2
        Subtype           : 1
        Interface Subtype : ifIndex(2)
        OID               : 1.3.6.1.2.1.31.1.1.1.1.2

Organization Info
       OUI      : 0.12.f
    ''',
    "ge-0/0/4.0": '''
Parent Interface   : -
Local Port ID      : 509
Ageout Count       : 0

Neighbour Information:
Chassis type       : Mac address
Chassis ID         : 00:10:7f:8d:91:ed
Port type          : Mac address
Port ID            : 00:10:7f:8d:91:ed
Port description   : eth0
System name        : ITC-0173-PROC1

System Description :  Linux 3.10.33 #4 SMP PREEMPT Wed Mar 21 23:16:38 EDT 2018 armv7l

System capabilities
        Supported  : Bridge WLAN Access Point Router Station Only
        Enabled    : Station Only

Management Info
        Type              : IPv4
        Address           : 10.126.4.79
        Port ID           : 2
        Subtype           : 1
        Interface Subtype : ifIndex(2)
        OID               : 1.3.6.1.2.1.31.1.1.1.1.2

Organization Info
       OUI      : 0.12.f
    ''',
    "ge-0/0/6.0": '''
Parent Interface   : -
Local Port ID      : 509
Ageout Count       : 0

Neighbour Information:
Chassis type       : Mac address
Chassis ID         : 00:10:7f:8d:91:ed
Port type          : Mac address
Port ID            : 00:10:7f:8d:91:ed
Port description   : eth0
System name        : ITC-0173-DISP1

System Description :  Linux 3.10.33 #4 SMP PREEMPT Wed Mar 21 23:16:38 EDT 2018 armv7l

System capabilities
        Supported  : Bridge WLAN Access Point Router Station Only
        Enabled    : Station Only

Management Info
        Type              : IPv4
        Address           : 10.126.4.80
        Port ID           : 2
        Subtype           : 1
        Interface Subtype : ifIndex(2)
        OID               : 1.3.6.1.2.1.31.1.1.1.1.2

Organization Info
       OUI      : 0.12.f
    '''
}

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
ge-0/0/3.0         -                   12:34:56:78:9A:BC   151                ITC-0173-PROC1
ge-0/0/4.0         -                   12:34:56:78:9A:BC   151                ITC-0173-TP1
ge-0/0/6.0         -                   12:34:56:78:9A:BC   151                ITC-0173-DISP1
        """)
    elif command.startswith("show lldp interface"):
        comList = command.split(" ")
        if len(comList) == 4 and comList[3] in interfaceDict.keys():
            print(interfaceDict.get(comList[3]))
        else:
            print("[-] I_ERR: Interface unknown.")
    elif command == "quit":
        print("Goodbye.")
        run = False
    else:
        print("[-] Error: Unknown command.")

print("")