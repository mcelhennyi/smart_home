# Ian McElhenny
#

import sys
import bluetooth
import time
import struct

if sys.version < '3':
    input = raw_input

sock=bluetooth.BluetoothSocket(bluetooth.RFCOMM)

if len(sys.argv) < 2:
    print("usage: l2capclient.py <addr>")
    sys.exit(2)

bt_addr=sys.argv[1]
port = 0x01

print("trying to connect to %s on PSM 0x%X" % (bt_addr, port))

sock.connect((bt_addr, port))

print("connected. listening...")
while True:
    # data = input()
    # if(len(data) == 0): break
    # sock.send(data)
    try:
        # sock.send("")

        data = sock.recv(1024)
        # val_name, val_num = parse_buf(buf, data)

        # short_val = struct.unpack('h', data)

        print "Value: " + str(data)

        time.sleep(.01)
    except KeyboardInterrupt:
        break

sock.close()