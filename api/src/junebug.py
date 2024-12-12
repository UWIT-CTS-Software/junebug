import os

filepath = __file__.removesuffix("junebug.py")

os.system(f"gcc {filepath}pull_dhcp.c -o {filepath}../bin/pull_dhcp.out")
os.system(f"{filepath}../bin/pull_dhcp.out")