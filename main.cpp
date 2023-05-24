//---------------------------------------------------------------------
//	scab
//---------------------------------------------------------------------
//#pragma comment(lib, "portaudio_x64.lib")
#define DR_WAV_IMPLEMENTATION
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <math.h>
#include "portaudio.h"
#include "./dr_wav.h"

//#define ENTRIG
//#define TRIG_DEV
// TRIG_DEV
// 0 : NIDAQ
// 1 : ButtonBox

using namespace std;

#if ENTRIG == 1
#if TRIG_DEV == 0
	#include "NIDAQmx.h"
#endif
#if TRIG_DEV == 1
	#include "../Buttonbox-C/Buttonbox.h"
#endif
#endif

void add_stim(float* y, float* stim, int N) {
	for (int i = 0; i < N; i++) {
		*(y + i) += *(stim + i);
	}
}

float abs_fmax(float* x, int N) {

	float y = fabsf(x[0]);
	for (int i = 1; i < N; i++) {
		if (y < fabsf(x[i])) {
			y = fabsf(x[i]);
		}
	}
	return y;
}

int imax(int* x, int N) {
	int y = x[0];
	for (int i = 1; i < N; i++) {
		if (y < x[i]) {
			y = x[i];
		}
	}
	return y;
}

class AudioHandler
{
	int N;
	int Fs;
	float* csv_data;
public:
	AudioHandler();
	~AudioHandler();
	void load_csv(const char* filename, int Fs);
	void gen_array(float** tones, float* len_tones);
	int n_ch;
	float time;
	float** data;
	int* trig;
	int length;
};

AudioHandler::AudioHandler(){
	
}

AudioHandler::~AudioHandler(){

}

void AudioHandler::load_csv(const char* filename, int Fs) {
	std::ifstream ifs;
	ifs.open(filename);

	this->Fs = Fs;

	char buf[256];
	int linenum = 0;

	ifs.getline(buf, sizeof(buf));
	linenum = (int)atof(buf);
	
	ifs.getline(buf, sizeof(buf));
	this->n_ch = (int)atof(buf);

	int len_csv = linenum - 2;

	this->csv_data = new float[len_csv];
	
	for (int i = 0; i < len_csv; i++) {
		ifs.getline(buf, sizeof(buf));
		this->csv_data[i] = (float)atof(buf);
	}

	this->N = (int)(len_csv / 4);
	
}

void AudioHandler::gen_array(float** tones, float* len_tones) {
	// unit of len_tones : seconds

	float* t_csv = new float[this->N];
	int* ch_csv = new int[this->N];
	int* stim_csv = new int[this->N];
	int* trig_csv = new int[this->N];

	int idx;
	for (int i = 0; i < this->N; i++) {
		idx = i * 4;
		t_csv[i] = this->csv_data[idx++];
		ch_csv[i] = this->csv_data[idx++];
		stim_csv[i] = this->csv_data[idx++];
		trig_csv[i] = this->csv_data[idx];
	}
	

	delete [] this->csv_data;

	int Fs = this->Fs;
	int n_ch = this->n_ch;
	

	//float** y_mat = new float* [n_ch];
	float** y_mat = (float**)malloc(sizeof(float*) * n_ch);

	int n_stim = imax(stim_csv, this->N) + 1;
	this->time = abs_fmax(t_csv, this->N) + abs_fmax(len_tones, n_stim);

	int n_samples = (this->time + 10) * Fs;

	//--------------
	// y

	for (int i = 0; i < n_ch; i++) {
		//y_mat[i] = new float[n_samples];
		y_mat[i] = (float*)malloc(sizeof(float) * n_samples);
		for (int j = 0; j < n_samples; j++) {
			y_mat[i][j] = 0;
		}
	}

	for (int i = 0; i < this->N; i++) {
		idx = (int)(t_csv[i] * Fs);
		add_stim(&(y_mat[ch_csv[i]][idx]), tones[stim_csv[i]], (int)(len_tones[stim_csv[i]] * Fs));
	}
	

	//--------------
	// trig

	//int* trig_arr = new int[n_samples];
	int* trig_arr = (int*)malloc(sizeof(int) * n_samples);
	for (int i = 0; i < n_samples; i++) {
		trig_arr[i] = 0;
	}

	for (int i = 0; i < N; i++) {
		idx = t_csv[i] * Fs;
		trig_arr[idx] = trig_csv[i];
	}

	float* max_ch = new float[n_ch];
	for (int i = 0; i < n_ch; i++) {
		max_ch[i] = abs_fmax(y_mat[i], n_samples);
	}

	float max_amp = abs_fmax(max_ch, n_ch);
	if (max_amp > 1.0) {
		std::cout << "Warning : amplitude exceeds 1, max amplitude : " << max_amp << std::endl;
	}

	this->data = y_mat;
	this->trig = trig_arr;
	this->length = idx;
}

void QueryPerformanceCounterSleep(int time_millisec, LARGE_INTEGER freq) {
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);
	QueryPerformanceCounter(&end);
	while (((double)(end.QuadPart - start.QuadPart) / freq.QuadPart) <= time_millisec / 1000.0) {
		QueryPerformanceCounter(&end);
	}
}

LARGE_INTEGER getQueryPerformanceCounter(LARGE_INTEGER* now) {
	QueryPerformanceCounter(now);
	return *now;
}


typedef struct PADATA {
	float** y;
	int* trig;
	int n_ch;
	int idx = 0;
	int current_trig = 0;
}padata;

static int dsp(const void *inputBuffer, 
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo *timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData
) {
	padata *data = (padata *)userData;
	float *out = (float *)outputBuffer;

	for (int i = 0; i < framesPerBuffer; i++) {
		for (int ch = 0; ch < data->n_ch; ch++) {
			*out++ = data->y[ch][data->idx];
		}
#if ENTRIG == 1
		if (data->trig[data->idx] != 0) {
			data->current_trig = data->trig[data->idx];
		}
#endif
		data->idx++;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	
	char* audio_data_csv = argv[1];
	char* audio_files_csv = argv[2];
	int Fs = atoi(argv[3]); 
	int frames_per_buffer = atoi(argv[4]);
	
	
	cout << "Audio Data CSV file name : " << audio_data_csv << endl;
	cout << "Audio Files CSV file name : " << audio_files_csv << endl;
	cout << "Fs : " << Fs << endl;
	cout << "Frames per buffer : " << frames_per_buffer << endl;
	//cout << frames_per_buffer << endl;

#if ENTRIG == 1
#if TRIG_DEV == 1
	char* bb_port = argv[5];
	cout << "ButtonBox Port : " << bb_port << endl;
#endif
#if TRIG_DEV == 0
	char* ni_port = argv[5];
	cout << "NI-DAQ Port : " << ni_port << endl;
#endif
#endif


#if ENTRIG == 1
#if TRIG_DEV == 0
	//-------------------------------------------------------------------------------------------------
	// NI DAQ INITIALIZATION

	int error = 0;
	TaskHandle taskHandle = 0;
	uInt8 trig = 0;
	char errBuff[2048] = { '\0' };
	int32 written;

	// DAQmx Configure Code
	DAQmxCreateTask("", &taskHandle);
	//DAQmxCreateDOChan(taskHandle, "Dev1/port0", "", DAQmx_Val_ChanForAllLines);
	DAQmxCreateDOChan(taskHandle, ni_port, "", DAQmx_Val_ChanForAllLines);

	// DAQmx Start Code
	DAQmxStartTask(taskHandle);

	// DAQmx Write Code
	DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);

	//-------------------------------------------------------------------------------------------------

#endif
#if TRIG_DEV == 1
	int trig = 0;
	Buttonbox bb;
	bb.open(bb_port, 3);
	bb.sendMarker(0);
	cout << "Button Box was connected successfully" << endl;
#endif
#endif
	
	//-----------------------------------------------------------
	// read wav file

	unsigned int channels;
	unsigned int sampleRate;

	std::ifstream ifs;
	ifs.open(audio_files_csv);
	char buf[512];
	ifs.getline(buf, sizeof(buf));
	int n_stim = (int)atof(buf);

	drwav_uint64* n_samples = new drwav_uint64[n_stim];
	float* len_tones = new float[n_stim];
	float** wav_data = new float*[n_stim];
	char filename[n_stim][256];

	for (int i = 0; i < n_stim; i++) {
		ifs.getline(buf, sizeof(buf));
		strcpy(filename[i], buf);
	}

	for (int i = 0; i < n_stim; i++) {
		cout << "filename " << i << " : " << filename[i] << endl;
		wav_data[i] = drwav_open_file_and_read_pcm_frames_f32(filename[i], &channels, &sampleRate, &n_samples[i], NULL);
	}

	for (int i = 0; i < n_stim; i++){
		len_tones[i] = (float)n_samples[i] / Fs;
	}

	AudioHandler audio_handler;
	audio_handler.load_csv(audio_data_csv, Fs);
	audio_handler.gen_array(wav_data, len_tones);

	cout << "CSV file was loaded." << endl;

	//-------------------------------------------------------------------------------------------------
	// Start

	PaStreamParameters outParam;
	PaStream* stream;
	PaError err;
	padata data;

	int n_ch = audio_handler.n_ch;
	cout << "n_ch : " << n_ch << endl;
	cout << "time : " << audio_handler.time << endl;
	

	data.n_ch = n_ch;
	data.y = audio_handler.data;
	data.trig = audio_handler.trig;

	//-------------------------------------------------------------------------------------------------
	// Initializing Port Audio
	Pa_Initialize();
	outParam.device = Pa_GetDefaultOutputDevice();
	outParam.channelCount = n_ch;

	outParam.sampleFormat = paFloat32;
	outParam.suggestedLatency = Pa_GetDeviceInfo(outParam.device)->defaultLowOutputLatency;
	outParam.hostApiSpecificStreamInfo = NULL;

	Pa_OpenStream(
		&stream,
		NULL,
		&outParam,
		Fs,
		frames_per_buffer,
		paClipOff,
		dsp,
		&data);

	//cout << Pa_GetDeviceInfo(outParam.device)->name << endl;
	cout << "API : " << Pa_GetHostApiInfo(Pa_GetDefaultHostApi())->name << endl;
	if (strcmp(Pa_GetHostApiInfo(Pa_GetDefaultHostApi())->name, "ASIO") != 0) {
		cout << "Warning : This session is NOT running on ASIO." << endl;
	}

	//-----------------------------------------------------------
	// initialize query performance counter
	LARGE_INTEGER start, now, clock;
	QueryPerformanceFrequency(&clock);
	//-----------------------------------------------------------
	QueryPerformanceCounterSleep(1000, clock);
	//-----------------------------------------------------------
	Pa_StartStream(stream);  // Start Port Audio
	//-----------------------------------------------------------

	cout << "Started" << endl;

	QueryPerformanceCounter(&start);
	QueryPerformanceCounter(&now);
	while (((double)(now.QuadPart - start.QuadPart) / clock.QuadPart) <= audio_handler.time) {

#if ENTRIG == 1
		if (data.current_trig != 0) {
			trig = data.current_trig;
#if TRIG_DEV == 0
			DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
#endif
#if TRIG_DEV == 1
			bb.sendMarker(trig);
#endif
			trig = 0;
			data.current_trig = 0;
			QueryPerformanceCounterSleep(5, clock);
#if TRIG_DEV == 0
			DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
#endif
#if TRIG_DEV == 1
			bb.sendMarker(trig);
#endif
		}
#endif
		QueryPerformanceCounter(&now);
	}

	QueryPerformanceCounterSleep(1000, clock);

#if ENTRIG == 1
	trig = 0;
#if TRIG_DEV == 0
	DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
#endif
#if TRIG_DEV == 1
	bb.sendMarker(trig);	
#endif
#endif

#if 1
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	Pa_Terminate();

#if ENTRIG == 1
#if TRIG_DEV == 1
	bb.close();
#endif
#endif

#endif

	return 0;
}