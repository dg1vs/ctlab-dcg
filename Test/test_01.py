#! /usr/bin/python

#
import time
import argparse
#
import ctlab
import ctlab_helper

print("Test01")
print("Just stupid looping from 0...255")

# From Config-File
(serial_port, unic_config) = ctlab_helper.read_configfile('config.ini')

lab = ctlab.ctlab(serial_port)
lab.check_devices(verbose=True)

print("---------------------------------------")
result = lab.send_command_result(lab.dcg2, "idn?")
print(result)
print("---------------------------------------")



for index in range(100,110):
    cmd = "%i?" % index
    result = lab.send_command_result(lab.dcg2, cmd)
    print("%5d --> \t %s" % (index, result))
    time.sleep(0.1)

