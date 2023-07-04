# scab-c

# Arguments
1 : audio data csv  
2 : audio files csv  
3 : Fs  
4 : Frames per buffer  
5 : portnum  

# compile

## macro
| macro | description |
| ----- | ----------- |
| ENTRIG | Enable trigger if 1, Disable if 0 |
| TRIG_DEV | Use Nidaq if 0, Buttonbox if 1 |


```
# compile without trigger
g++ ./main.cpp -L./libs -lportaudio_x64 -o ./build/scab_notrigger.exe -DENTRIG=0

# compile with nidaq trigger
g++ ./main.cpp -L./libs -lportaudio_x64 -lNIDAQmx -o ./build/scab_nidaq.exe -DENTRIG=1 -DTRIG_DEV=0

# compile with Buttonbox trigger
g++ ./main.cpp ./libs/serialib.o -L./libs -lportaudio_x64 -o ./build/scab_buttonbox.exe -DENTRIG=1 -DTRIG_DEV=1

# compile with native environment
g++ -Ofast -mtune=native -march=native -mfpmath=both -std=c++14 ./main.cpp -L./libs -lportaudio_x64 -lNIDAQmx -o ./build/main.exe

# compile with native environment (nidaq trigger)
g++ ./main.cpp -L./libs -lportaudio_x64 -lNIDAQmx -mtune=native -march=native -mfpmath=both -o ./build/scab_nidaq.exe -DENTRIG=1 -DTRIG_DEV=0

```
