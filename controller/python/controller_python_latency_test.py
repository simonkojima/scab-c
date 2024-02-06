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

scab = "lsl"
#scab = "nidaq"

soa = 0.6

Fs = 44100
frames_per_buffer = 48

#itrs = 11
itrs = 200
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
for idx, m in enumerate(plan):
    val = list()
    val.append(soa*(idx+1)) # time (seconds)
    val.append(0) # channel
    val.append(0) # file id
    val.append(1) # trigger
    audio_csv_data.append(val)
    #if idx == 3:
    #    break

#print(audio_csv_data)
#audio_csv_data.insert(0, [1])
#audio_csv_data.insert(0, [len(audio_csv_data)+1])

files_csv_data = list()
files_csv_data.append(os.path.join(scab_dir, "misc", "audio", "500_latency_test.wav"))
#files_csv_data.append(os.path.join(scab_dir, "misc", "audio", "1200.wav"))

json_data = dict()
json_data['sequence'] = audio_csv_data
json_data['files'] = files_csv_data
json_data['n_channels'] = 1
json_data['sample_rate'] = 44100
json_data['frames_per_buffer'] = frames_per_buffer


target_ip = "127.0.0.1"
target_port = 65500
#buffer_size = 4096
tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#tcp_client.settimeout(0.01)

def play():
    try: 
        if scab == "lsl":
            command = [os.path.join(scab_dir, "build", "scab_lsl.exe")]
        elif scab == "nidaq":
            command = [os.path.join(scab_dir, "build", "scab_nidaq.exe")]
            json_data["ni_port"] = "Dev1/port0"
        p = subprocess.Popen(command)
        tcp_client.connect((target_ip,target_port))
        
        input("\nPress Any Key to Start.")
        tcp_client.send(json.dumps(json_data).encode())
        response = tcp_client.recv(4096).decode('utf-8')
        if response == 'end':
            return

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