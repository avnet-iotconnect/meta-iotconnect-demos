#!/usr/bin/env python3
'''
    Basic sample loading credentials from file and sending data to endpoint
'''
import time
import sys
from model.json_device import JsonDevice


def main(argv):
    '''Main function'''
    
    CREDENTIALS_PATH = argv[1:][0]
    device = JsonDevice(CREDENTIALS_PATH)
    device.connect()

    while True:
        if device.needs_exit and not device.in_ota:
            print("OTA requested exit, exiting")
            break
        
        data_sent = device.send_device_states()
        print(data_sent)
        time.sleep(10)

if __name__ == "__main__":
    main(sys.argv)
