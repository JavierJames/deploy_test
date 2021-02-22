#!/usr/bin/python3.6

import argparse
import os, glob


ESP_REPO_DIR = '/home/javier/repo/esp32-firmware/'
# PIC_FW_FOLDER = 

ESP_FW_FOLDER = ESP_REPO_DIR + 'build/'
ESP_FW_NAME_DEF = 'esp32-firmware'
ESP_FW_FORMAT = '.bin'
ESP_FW_FULL_NAME = ESP_FW_NAME_DEF + ESP_FW_FORMAT
    
target_speaker = 'SAT' 
target_mcu = 'ESP'   


fw_info_dic = {
    "dir_entry":"",
    "fw_name": "",
    "fw_path:": "",
    "target_speaker": "",
    "target_mcu": "",
    "commit_sha_long": "",
    "commit_sha_short": ""
}

 

def _get_latest_commmit():
    #Get commit ID and remove trailing '\n'
    stream = os.popen('git rev-parse HEAD') 
    cur_commit_sha = stream.read().rstrip('\n')
  
    stream = os.popen('git rev-parse --short HEAD') 
    cur_commit_sha_short = stream.read().rstrip('\n')

    return cur_commit_sha, cur_commit_sha_short



def encode_fw(fw_dic):
    print('Encoding firmware')

    print('fw_path:',fw_dic["fw_path"])
    target_name = ''


    #check if file path exists
    #See if file is present 
    try:
        if os.path.exists(fw_dic["fw_path"]):
            print("file exists")

            _src_fw_name = fw_dic['fw_name'] 
            _src_fw_path = fw_dic['fw_path']
            
            #get current commit
            fw_dic["commit_sha_long"], fw_dic["cur_commit_sha_short"] = _get_latest_commmit()

            desired_name = fw_dic['target_speaker'] + '-' + fw_dic['target_mcu'] + '-' + fw_dic['cur_commit_sha_short']
            desired_name = desired_name.lower()

            #update fw name and path 
            fw_dic['fw_name'] = fw_dic['fw_name'].replace( ESP_FW_FULL_NAME,desired_name + ESP_FW_FORMAT )
            fw_dic['fw_path'] = fw_dic['fw_path'].replace( ESP_FW_FULL_NAME,desired_name + ESP_FW_FORMAT)

            #change actual file
            os.rename(_src_fw_path,fw_dic['fw_path'])

        else:
            print('file doesn\'t not exists')

    except OSError:
        print('Could not process renaming file')
    except:
        print('Exception')


def get_fw():
    """
    Search for ESP fw for release
    """
    os.chdir(ESP_FW_FOLDER)

    esp_fw_path = ''
    entry_fw = ''
    basepath = ESP_FW_FOLDER
    try:

        with os.scandir(basepath) as entries:
            for entry in entries:
                if entry.is_file():
                    filename = entry.name
                    if filename.endswith(ESP_FW_FORMAT) and filename.startswith(ESP_FW_NAME_DEF):
                        if ESP_FW_NAME_DEF +  ESP_FW_FORMAT in filename:
                            esp_fw_path = entry.path
                            entry_fw = entry 
    
    except:
        print("Exception. could not open file")


    return entry_fw 


def main():
    global target_speaker
    global target_mcu
    global fw_info_dic

    parser = argparse.ArgumentParser(description='Encode file name')
    parser.add_argument('-s', '--speaker_type', default='SAT', choices=['SAT','SUB'],
                        help='Specify if target is SAT or SUB')
    parser.add_argument('-m', '--mcu_type', default='ESP', choices=['ESP','PIC'],
                        help='Specify the target MCU. ESP or PIC')

    args = parser.parse_args()

    target_speaker = args.speaker_type
    target_mcu = args.mcu_type
    print('target speaker:', target_speaker)
    print('target MCU:', target_mcu)


    #Get the firmware
    entry_fw = get_fw()

    if( entry_fw):
        #Update firmware info dictionary object
        fw_info_dic["dir_entry"] = entry_fw
        fw_info_dic["fw_name"] = entry_fw.name
        fw_info_dic["fw_path"] = entry_fw.path
        fw_info_dic["target_speaker"] = target_speaker
        fw_info_dic["target_mcu"] = target_mcu

        encode_fw(fw_info_dic)

    else:
        print("Could not open FW. Program not running")


if __name__ == '__main__':
    main()