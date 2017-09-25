import sys
import bluetooth
import time
import struct
import paho.mqtt.client as mqtt

#Enums
START_BYTE = 5
END_BYTE = 2
UNDEF = 0
PIT = 1
MEAT = 2

# States
START = 0
LENGTH = 1
DATA_ENUM = 2
DATA_VAL = 3
END = 4


# Know vals
GREATEST_LENGTH = 10  # maybe 10 if padding enabled
MAX_DATA_VAL = 700  # verify the data field
MIN_DATA_VAL = 0  # verify the data field

BLUETOOTH_ADDR = "20:16:01:20:26:16"
MQTT_BROKER_ADDR = "192.168.1.202"

MQTT_DOMAIN_TOPIC = "smoker/"

class SmokerInterface:
    def __init__(self):
        self.buf = ''
        self.current_state = 0
        self.buf_indx = 0

        self.packet_length = 0
        self.data_enum = 0
        self.data_val = 0
        self.data_ready = False
        self.data_name = "UNDEF"

        # setup mqtt
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.connect(MQTT_BROKER_ADDR, port=1883, keepalive=60, bind_address="")

        # Blue tooth socket stuff
        self.sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
        port = 0x01
        self.sock.connect((BLUETOOTH_ADDR, port))
        print("trying to connect to %s on PSM 0x%X" % (BLUETOOTH_ADDR, port))
        print("connected. listening...")

    def main(self):
        while True:
            try:
                data = self.sock.recv(1)
                # print "Received: " + str(bytes(ord(data)))
                self.parse_buf(data)

                if self.data_ready:
                    print "Received( " + self.data_name + ": " + str(self.data_val) + ")"
                    self.data_ready = False
                    self.publish_mqtt(self.data_name, self.data_val)

            except KeyboardInterrupt:
                break

        self.sock.close()

    def publish_mqtt(self, topic, value):
        self.mqtt_client.publish(MQTT_DOMAIN_TOPIC + topic, payload=value)

    def parse_buf(self, new_buf):

        # Append new_buf to end of buf
        self.buf += new_buf

        # Make sure we dont jump out of bounds
        if self.buf_indx < len(self.buf):
            # Jump through states
            if self.current_state == START:
                # Search for start byte
                if int(bytes(ord(self.buf[0]))) == START_BYTE:
                    # we found the start byte, kick off the rest of the search
                    self.advance_state(1)  # we used one byte
                else:
                    # Delete this byte, and continue on...this isnt it
                    self.restart()
                    print "failed start"

            elif self.current_state == LENGTH:
                # Verify length field
                if self.buf_indx+1 < len(self.buf):  # +1 because we know length is a short, so we need ind and next ind
                    # If we know that we have two byte here, unpack it as length field
                    self.packet_length, = struct.unpack('h', self.buf[self.buf_indx:self.buf_indx+2])
                    if self.packet_length <= GREATEST_LENGTH:
                        self.advance_state(2)  # We used two bytes
                    else:
                        # its a bad length, restart the process
                        self.restart()
                        print "failed len"

            elif self.current_state == DATA_ENUM:
                # Get data enum
                if self.buf_indx + 1 < len(self.buf):  # +1 because we know enum is a short, so we need ind and next ind
                    # If we know that we have two byte here, unpack it as enum field
                    self.data_enum, = struct.unpack('h', self.buf[self.buf_indx:self.buf_indx + 2])
                    if self.check_for_enum(self.data_enum):
                        # We found it, move on
                        self.advance_state(2)  # we used two bytes here, and we need to align the float +1
                    else:
                        # Bad field, start over
                        self.restart()
                        print "failed enum"

            elif self.current_state == DATA_VAL:
                # Get data
                if self.buf_indx + 3 < len(self.buf):  # +3 because we know data field is a float
                    # If we know that we have 4 byte here, unpack it as data field
                    self.data_val, = struct.unpack('f', self.buf[self.buf_indx:self.buf_indx + 4])
                    if self.data_val > MIN_DATA_VAL and self.data_val < MAX_DATA_VAL:
                        # looks like a good number, move on
                        self.advance_state(4)  # we used 4 bytes this time
                    else:
                        # bad value, restart
                        self.restart()
                        # print "failed data"
                        print "Make sure probs are attached"

            elif self.current_state == END:
                # Find end byte (aka validate all info above)
                if int(bytes(ord(self.buf[self.buf_indx]))) == END_BYTE:
                    # We found the end byte, looks like good data to use
                    self.data_ready = True
                    self.post_found_restart()  # rips the packet off the buffer and reset tracking vars
                else:
                    # bad end byte start over
                    self.restart()
                    print "failed end"

        else:
            print "Passed parsing, no new data."

    def advance_state(self, buf_inc):
        self.current_state += 1
        self.buf_indx += buf_inc

    def restart(self, del_len=1):  # defaults to one byte, for restarting search sequence on a fail case
        # reset state data
        self.current_state = 0
        self.buf_indx = 0
        self.buf = self.buf[del_len:]  # Rip off the first byte and try sequence again

        # reset found vals
        if del_len == 1:  # Make sure we dont reset flag before caller func can grab data
            self.packet_length = 0
            self.data_enum = 0
            self.data_val = 0
            self.data_ready = False
            self.data_name = "UNDEF"

    def post_found_restart(self):
        self.restart(self.packet_length)

    def check_for_enum(self, found_enum):
        found = False

        # search if this matches what we know
        if found_enum == UNDEF:
            found = True
            self.data_name = "UNDEF"
        if found_enum == MEAT:
            found = True
            self.data_name = "meat_temp"
        if found_enum == PIT:
            found = True
            self.data_name = "pit_temp"

        return found


if __name__ == "__main__":
    si = SmokerInterface()
    si.main()