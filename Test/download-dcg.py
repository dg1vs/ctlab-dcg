#! /usr/bin/python

#
import time
import json
import argparse
#
import ctlab
import ctlab_helper


def get_string_from_result(text):
    pos = text.find("=")
    return text[pos + 1:]


def get_file_name_from_result(text):
    t1 = get_string_from_result(text)
    t2 = t1.replace("\"","")
    return t2


def get_float_value_from_result(text):
    pos = text.find("=")
    return float(text[pos + 1:])


def get_int_value_from_result(text):
    pos = text.find("=")
    return int(float(text[pos + 1:]))


def get_metadata(lab):
    config['META'] = [] 
    # IDN
    result = lab.send_command_result_str(lab.dcg2, "idn?")
    res = get_string_from_result(result)
    print("  254 --> \t",  res)
    config['META'].append({'index': 254, 'value': res, })
   
    # TMP -- temperature
    result = lab.send_command_result_str(lab.dcg2, "233?")
    res = get_float_value_from_result(result)
    print("  233 --> \t",  res)
    config['META'].append({'index': 233, 'value': res, })
    

def get_value(idx, name, type, lab):
    config[name] = [] 
    cmd = str(idx) + "?" 
    result = lab.send_command_result_str(lab.dcg2, cmd)
    
    if (type =='int'): 
        res = get_int_value_from_result(result)
    elif  (type =='float'):   
        res = get_float_value_from_result(result)
    else:   
        res ="Invalid stuff";

    print("%5d --> \t %s" % (idx, res))
    config[name].append({  
        'index': idx,
        'value': res,
    })
    time.sleep(0.1)  


def get_configdata(lab):
    get_value(100, 'Offset_Voltages_DAC_low', 'int', lab)
    get_value(101, 'Offset_Voltages_DAC_high', 'int', lab)

    get_value(102, 'Offset_Current_DAC_2mA', 'int', lab)
    get_value(103, 'Offset_Current_DAC_20mA', 'int', lab)
    get_value(104, 'Offset_Current_DAC_200mA', 'int', lab)
    get_value(105, 'Offset_Current_DAC_2A', 'int', lab)

    get_value(110, 'Offset_Voltages_ADC_low', 'int', lab)
    get_value(111, 'Offset_Voltages_ADC_high', 'int', lab)

    get_value(112, 'Offset_Current_ADC_2mA', 'int', lab)
    get_value(113, 'Offset_Current_ADC_20mA', 'int', lab)
    get_value(114, 'Offset_Current_ADC_200mA', 'int', lab)
    get_value(115, 'Offset_Current_ADC_2A', 'int', lab)

    get_value(200, 'Scale_Voltages_DAC_low', 'float', lab)
    get_value(201, 'Scale_Voltages_DAC_high', 'float', lab)

    get_value(202, 'Scale_Current_DAC_2mA', 'float', lab)
    get_value(203, 'Scale_Current_DAC_20mA', 'float', lab)
    get_value(204, 'Scale_Current_DAC_200mA', 'float', lab)
    get_value(205, 'Scale_Current_DAC_2A', 'float', lab)
    
    get_value(210, 'Scale_Voltages_ADC_low', 'float', lab)
    get_value(211, 'Scale_Voltages_ADC_high', 'float', lab)

    get_value(212, 'Scale_Current_ADC_2mA', 'float', lab)
    get_value(213, 'Scale_Current_ADC_20mA', 'float', lab)
    get_value(214, 'Scale_Current_ADC_200mA', 'float', lab)
    get_value(215, 'Scale_Current_ADC_2A', 'float', lab)

    get_value(150, 'Default_Output_Voltage', 'float', lab)
    get_value(151, 'Default_Output_Current', 'float', lab)
    get_value(156, 'U_max_DCG', 'float', lab)
    get_value(171, 'Fan-Switching-Temperature', 'float', lab)


######
config = {}


def main():
    print("Download Config-data from DCG2 into a JSON file")
    
    # From Config-File
    (serial_port, unic_config) = ctlab_helper.read_configfile('config.ini')
    # Parse the cmd-line
    parser = argparse.ArgumentParser(description='Dump the UNI-C Config data into a json file.', prefix_chars='-')
    parser.add_argument("-p", "--port", help="Used port number")
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

    # open the serial_port
    lab = ctlab.ctlab(serial_port)
    lab.check_devices(verbose=True)

    get_metadata(lab)
    get_configdata(lab)

    #print (config)

    with open(unic_config, 'w') as outfile:  
        json.dump(config, outfile, indent=2)


if __name__ == "__main__":
    main()
