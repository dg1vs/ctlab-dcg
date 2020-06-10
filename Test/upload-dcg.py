#! /usr/bin/python

#
import time
import json
import argparse
#
import ctlab
import ctlab_helper

def write_wen_cmd(lab):
    result = lab.send_command_result_str(lab.dcg2, 'wen=1!')
    print(result)

def write_config_data(config, lab):
        #idx = 156
        #val = 3.4
        #write_wen_cmd(lab)
        #cmd = str(idx) + '=' + str(val) + '!' 
        #print (cmd)
        #
        #result = lab.send_command_result_str(lab.dcg2, cmd)
        #print(result)       
        
        # iterate the Dictionary
        for ii in config:
            if ii != 'META':
                print(config[ii])
                # Retrieve one Dictonry Entry
                xxx = config[ii]
                # Retrieve the first an only Element of the List
                aa = xxx[0]
                idx = aa['index']
                val = aa['value']
                #print(idx, val)
                
                write_wen_cmd(lab)
                cmd = str(idx) + '=' + str(val) + '!' 
                time.sleep(0.1)  
                print (cmd)
                result = lab.send_command_result_str(lab.dcg2, cmd)
                time.sleep(0.1)  
                #print(result)    



def main():

    print("Upload Config-data to DCG from JSON file")

    # From Config-File
    (serial_port, unic_config) = ctlab_helper.read_configfile('config.ini')
    # Parse the cmd-line
    parser = argparse.ArgumentParser(description='Upload the UNI-C Config data from a json file.', prefix_chars='-')
    parser.add_argument("-p", "--port", help="Used port nummer")
    parser.add_argument("-f", "--file", help="Filename config json file")
    args = parser.parse_args()
    # Use the old stuff, if cmd-line is empty
    if args.port is not None:
        serial_port = args.port
    if args.file is not None:
        unic_config = args.file
    # test
    print('port =', serial_port)
    print('file =', unic_config)

    # open the serialport
    #lab = 1
    lab = ctlab.ctlab(serial_port)
    lab.check_devices(verbose=True)

    with open(unic_config) as json_file:  
        config = json.load(json_file)

    write_wen_cmd(lab)

    write_config_data(config, lab)
    

main()