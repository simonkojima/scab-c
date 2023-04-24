//---------------------------------------------------------------------
//	scab
//---------------------------------------------------------------------
//#pragma comment(lib, "portaudio_x64.lib")
#define DR_WAV_IMPLEMENTATION
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <math.h>
//#include <random>
#include "portaudio.h"
#include "./dr_wav.h"


#define ENTRIG 0
#define FS 44100
#define FRAMES_PER_BUFFER 48

using namespace std;

#if ENTRIG == 1
	#pragma comment(lib, "NIDAQmx.lib")
	#include "NIDAQmx.h"
#endif

void add_stim(float* y, float* stim, int N) {
	cout << N << endl;
	for (int i = 0; i < N; i++) {
		cout << i << endl;
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

class siSequencer
{
	int N;
	int Fs;
	float* csv_data;
public:
	siSequencer();
	~siSequencer();
	void load_csv(const char* filename, int Fs);
	void gen_array(float** tones, float* len_tones);
	int n_ch;
	float time;
	float** data;
	int* trig;
	int length;
};

siSequencer::siSequencer(){
	
}

siSequencer::~siSequencer(){

}

void siSequencer::load_csv(const char* filename, int Fs) {
	cout << "load csv" << endl;
	std::ifstream ifs;
	cout << "file opening ..." << endl;
	ifs.open(filename);
	cout << "file was opened" << endl;

	this->Fs = Fs;

	char buf[256];
	int linenum = 0;

	while (ifs.getline(buf, sizeof(buf))) {
		linenum++;
	}

	ifs.clear();
	ifs.seekg(0, std::ios::beg);

	this->csv_data = new float[linenum];

	for (int i = 0; i < linenum; i++) {
		ifs.getline(buf, sizeof(buf));
		this->csv_data[i] = (float)atof(buf);
	}

	this->n_ch = (int)this->csv_data[0];
	this->csv_data = &(this->csv_data[1]);
	linenum--;
	this->N = linenum / 4;
}

void siSequencer::gen_array(float** tones, float* len_tones) {
	// unit of len_tones : seconds

	float* t_csv = new float[N];
	int* ch_csv = new int[N];
	int* stim_csv = new int[N];
	int* trig_csv = new int[N];
	
	cout << "All array was created" << endl;

	int idx;
	for (int i = 0; i < N; i++) {
		idx = i * 4;
		t_csv[i] = this->csv_data[idx++];
		ch_csv[i] = this->csv_data[idx++];
		stim_csv[i] = this->csv_data[idx++];
		trig_csv[i] = this->csv_data[idx];
	}

	this->csv_data = --this->csv_data;
	delete [] this->csv_data;

	int Fs = this->Fs;
	int n_ch = this->n_ch;

	//float** y_mat = new float* [n_ch];
	float** y_mat = (float**)malloc(sizeof(float*) * n_ch);
	cout << "y_mat was created." << endl;

	int n_stim = imax(stim_csv, N) + 1;
	this->time = abs_fmax(t_csv, N) + abs_fmax(len_tones, n_stim);

	int n_samples = (this->time + 10) * Fs;
	cout << "n_samples : " << n_samples << endl;

	//--------------
	// y

	for (int i = 0; i < n_ch; i++) {
		//y_mat[i] = new float[n_samples];
		y_mat[i] = (float*)malloc(sizeof(float) * n_samples);
		for (int j = 0; j < n_samples; j++) {
			y_mat[i][j] = 0;
		}
	}
	
	cout << "here" << endl;

	for (int i = 0; i < N; i++) {
		cout << i << endl;
		idx = (int)(t_csv[i] * Fs);
		cout << idx << endl;
		cout << "n_samples : " << len_tones[stim_csv[i]]*Fs << endl;
		add_stim(&(y_mat[ch_csv[i]][idx]), tones[stim_csv[i]], (int)(len_tones[stim_csv[i]] * Fs));
	}
	
	cout << "y was created" << endl;


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
	if (max_amp >= 1.0) {
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

int main(void) {

#if ENTRIG == 0
	int trig = 0;
#endif


#if ENTRIG == 1

	//-------------------------------------------------------------------------------------------------
	// NI DAQ INITIALIZATION

	int error = 0;
	TaskHandle taskHandle = 0;
	uInt8 trig = 0;
	char errBuff[2048] = { '\0' };
	int32 written;

	// DAQmx Configure Code
	DAQmxCreateTask("", &taskHandle);
	DAQmxCreateDOChan(taskHandle, "Dev1/port0", "", DAQmx_Val_ChanForAllLines);

	// DAQmx Start Code
	DAQmxStartTask(taskHandle);

	// DAQmx Write Code
	DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);


	//-------------------------------------------------------------------------------------------------

#endif
	//-----------------------------------------------------------
	// generate tones

	/*
	float freq[2] = { 1000, 1200 };
	float duration[2] = {0.2, 0.2};
	siTone tone[2];
	for (int i = 0; i < 2; i++) {
		tone[i].setFs(Fs);
		tone[i].genSineTone(freq[i], duration[i], 1, 0);
		tone[i].applyFade(0.01);
	}

	float** tones = new float*[2];
	for (int i = 0; i < 2; i++) {
		tones[i] = tone[i].get();
	}
	*/
	
	
	//-----------------------------------------------------------
	// read wav file

	unsigned int channels;
	unsigned int sampleRate;
	
	int n_stim = 2;

	drwav_uint64* n_samples = new drwav_uint64[n_stim];
	float* len_tones = new float[n_stim];
	float** wav_data = new float*[n_stim];
	const char** filename = new const char*[256];
	filename[0] = { "1000.wav" };
	filename[1] = { "1200.wav" };
	

	for (int i = 0; i < n_stim; i++) {
		wav_data[i] = drwav_open_file_and_read_pcm_frames_f32(filename[i], &channels, &sampleRate, &n_samples[i], NULL);
	}
	
	for (int i = 0; i < n_samples[0]; i++){
		cout << i << " : " << wav_data[0][i] << endl;
	}

	for (int i = 0; i < n_stim; i++){
		len_tones[i] = (float)n_samples[i] / FS;
		cout << "n_samples : " << n_samples[i] << ", length : " << len_tones[i] << endl;
	}
	
	cout << "here" << endl;


	siSequencer seq;
	cout << "seq instance was initialized" << endl;
	seq.load_csv("oddball.csv", FS);
	seq.gen_array(wav_data, len_tones);

	cout << "CSV file was loaded." << endl;

	//-------------------------------------------------------------------------------------------------
	// Start

	PaStreamParameters outParam;
	PaStream* stream;
	PaError err;
	padata data;

	int n_ch = seq.n_ch;
	cout << "ch : " << n_ch << endl;
	cout << "time : " << seq.time << endl;
	

	data.n_ch = n_ch;
	data.y = seq.data;
	data.trig = seq.trig;

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
		FS,
		FRAMES_PER_BUFFER,
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
	while (((double)(now.QuadPart - start.QuadPart) / clock.QuadPart) <= seq.time) {

#if ENTRIG == 1
		if (data.current_trig != 0) {
			trig = data.current_trig;
			DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
			trig = 0;
			data.current_trig = 0;
			QueryPerformanceCounterSleep(5, clock);
			DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
		}
#endif
		QueryPerformanceCounter(&now);
	}

	QueryPerformanceCounterSleep(1000, clock);

#if ENTRIG == 1
	trig = 0;
	DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
#endif

#if 1
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	Pa_Terminate();
#endif

	return 0;
}