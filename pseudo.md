# Junebug pseudocode #
## Goals ##
Reference the lldp advertisements of network switches to search for anomalies in IP assignments. Evaluation should be accomplished with understanding of what technology is found in the room, what its hostname should be, and which devices are expected to be visible to the AVVLAN. Junebug will provide a RESTful API (implemented in Python) to interface with the information collected by the master data collection module (built in C). An HTTP request from a client will return JSON in the form of a room's status, devices ranked by shortest uptime, or flagged offline devices.

## Program Flow ##
Python:
  - Initialize status table
  - Collect room configuration and switch type data
  - Initialize TCP listener threads for calls
    - When a call is made, verify headers and then parse command
      - ...?p={__arg1__:__value__,__arg2__:__value__...}&max=__number__&offset=__number__
        - Arguments:
          - rooms:[__room1__,__room2__, ...]
          - building:__building abbreviation__
          - uptime:__number__
          - alert:__low__|__medium__|__high__
    - return data as JSON
  - Initialize collector threads
    - Constantly running on a loop
    - Hostname configs and switch type passed as arguments
    - Pull lldp, send to python
    - Python parses and evaluates returning info
    - Store status data

C:
  - Collect arguments passed to it
  - Redirect stdout to a file for retrieval
  - Initialize SSH connection to target switch
  - Login, access lldp table from switch
  - If needed, iterate over table values to collect more detailed data.

Command flow: Jupiter Switches
```
Enter key
Username, enter
Password, enter
cli
show lldp neighbors
for neighbor in neighbors:
  show lldp neighbors interface <neighbor-port-id>
quit
exit
```

Command Flow: Netgear Switches
```
Username, enter
Password, enter
enable
Password, enter
show lldp interface all
for interface in interfaces:
  show lldp interface <interface unit/port id>
quit
```
