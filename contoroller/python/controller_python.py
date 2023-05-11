import os
import sys
import csv
import subprocess

import utils

home_dir = os.path.expanduser('~')

print(home_dir)

scab_dir = os.path.join(home_dir, 'git', 'scab-c')

target = 1
min_nstims_before_first_target = 4

soa = 0.6

Fs = 44100
frames_per_buffer = 1024

_plan = utils.generate_stimulation_plan(n_stim_types = 5, itrs = 10)
while True:
    if utils.check_min_nstims_before_first_target(_plan, target) < min_nstims_before_first_target:
        _plan = utils.generate_stimulation_plan(n_stim_types = 5, itrs = 10)
    else:
        break
            
print(_plan)

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
    audio_csv_data.append([soa*(idx+1)])
    audio_csv_data.append([0])
    audio_csv_data.append([m])
    audio_csv_data.append([m])

audio_csv_data.insert(0, [1])
audio_csv_data.insert(0, [len(audio_csv_data)+1])

files_csv_data = list()
files_csv_data.append([os.path.join(scab_dir, "1000.wav")])
files_csv_data.append([os.path.join(scab_dir, "1200.wav")])
files_csv_data.insert(0, [len(files_csv_data) + 1])

with open(os.path.join(scab_dir, "python_test_audio.csv"), 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(audio_csv_data)

with open(os.path.join(scab_dir, "python_test_files.csv"), 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(files_csv_data)


command = [os.path.join(scab_dir, "main.exe"),
           os.path.join(scab_dir, "python_test_audio.csv"),
           os.path.join(scab_dir, "python_test_files.csv"),
           str(Fs),
           str(frames_per_buffer)]

try: 
    p = subprocess.Popen(command, shell=True)
    while True:
        if p.poll() is not None:
            break
except:
    p.kill()
    print("process was killed.")


 
#sys.stdout.buffer.write(res.stdout)