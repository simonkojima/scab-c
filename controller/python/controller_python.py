import os
import sys
import socket
import csv
import subprocess
import threading
import json
import time

import utils

home_dir = os.path.expanduser('~')

#print(home_dir)

scab_dir = os.path.join(home_dir, 'git', 'scab-c')

target = 1
min_nstims_before_first_target = 4

soa = 0.2

Fs = 44100
frames_per_buffer = 1024

itrs = 30

_plan = utils.generate_stimulation_plan(n_stim_types = 5, itrs = itrs)
while True:
    if utils.check_min_nstims_before_first_target(_plan, target) < min_nstims_before_first_target:
        _plan = utils.generate_stimulation_plan(n_stim_types = 5, itrs = itrs)
    else:
        break
            
#print(_plan)
#print(len(_plan))

# TODO
# analyze targt-target distances

plan = list()
for m in _plan:
    if m == target:
        plan.append(1)
    else:
        plan.append(0)


audio_csv_data = list()
audio_csv_data.append([0, 0, -1, 200])
for idx, m in enumerate(plan):
    val = list()
    val.append(soa*(idx+1)) # time (seconds)
    val.append(0) # channel
    #if idx % 3 == 0:
    #    val.append(-1) # stim index
    #else:
    #    val.append(m) # stim index
    val.append(m)
    #val.append(m+1) # trigger
    if m == 0:
        val.append(1)
    elif m == 1:
        val.append(11)
    audio_csv_data.append(val)
    #if idx == 3:
    #    break
audio_csv_data.append([soa*(idx+2), 0, -1, 255])
#print(audio_csv_data)
#audio_csv_data.insert(0, [1])
#audio_csv_data.insert(0, [len(audio_csv_data)+1])

files_csv_data = list()
files_csv_data.append(os.path.join(scab_dir, "misc", "audio", "1000.wav"))
files_csv_data.append(os.path.join(scab_dir, "misc", "audio", "1200.wav"))

json_data = dict()
json_data['sequence'] = audio_csv_data
json_data['files'] = files_csv_data
json_data['n_channels'] = 1
json_data['sample_rate'] = 44100
json_data['frames_per_buffer'] = 512



target_ip = "127.0.0.1"
target_port = 49152
header_length = 64
#buffer_size = 4096
tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#tcp_client.settimeout(0.01)

def play():
    try: 
        command = [os.path.join(scab_dir, "build", "scab-lsl.exe")]
        p = subprocess.Popen(command, shell=True)
        tcp_client.connect((target_ip,target_port))
        time.sleep(1)
        
        while True:
            input("\nPress Any Key to Start.")
            tcp_client.send(len(json.dumps(json_data).encode('utf-8')).to_bytes(header_length, byteorder='little'))
            tcp_client.send(json.dumps(json_data).encode('utf-8'))

            msg_length = int.from_bytes(tcp_client.recv(header_length), 'little')
            message_recv = tcp_client.recv(msg_length).decode('utf-8')
            msg_json = json.loads(message_recv)
            print(msg_json)

    except:
        p.kill()
        print("process was killed.")
    
    

while True:
    thread = threading.Thread(target=play)
    thread.start()
    while True:
        time.sleep(1)
        #print(thread.is_alive())
exit()

try: 
    p = subprocess.Popen(command)
    print("scab was started")
    tcp_client.connect((target_ip,target_port))
    tcp_client.send(json.dumps(json_data).encode())

    while True:
        print("abc")
        response = tcp_client.recv(4096)
        print(response.decode('utf-8'))
        if p.poll() is not None:
            break
except:
    p.kill()
    print("process was killed.")

exit()

with open(os.path.join(scab_dir, "python_test_audio.csv"), 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(audio_csv_data)

with open(os.path.join(scab_dir, "python_test_files.csv"), 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(files_csv_data)


exit()



 
#sys.stdout.buffer.write(res.stdout)