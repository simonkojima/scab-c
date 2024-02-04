//---------------------------------------------------------------------
//	scab
//---------------------------------------------------------------------
//#pragma comment(lib, "portaudio_x64.lib")
#define DR_WAV_IMPLEMENTATION
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <math.h>

//#include <iostream>
//#include <sys/socket.h>
//#include <sys/types.h>
//#include <arpa/inet.h>
//#include <unistd.h>
//#include <string>
//#include <cstring>

#include "portaudio.h"
#include "dr_wav.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

//#define ENTRIG
//#define TRIG_DEV
// TRIG_DEV
// 0 : NIDAQ
// 1 : ButtonBox
// 2 : LSL

using namespace std;

#if ENTRIG == 1
#if TRIG_DEV == 0
	#include "NIDAQmx.h"
#endif
#if TRIG_DEV == 1
	#include "../Buttonbox-C/Buttonbox.h"
#endif
#if TRIG_DEV == 2
	#include "lsl_cpp.h"
#endif
#endif

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
	float* csv_data;
public:
	AudioHandler();
	~AudioHandler();
	void load_csv(const char* filename, int Fs);
	void gen_array_csv(float** tones, float* len_tones);
	void gen_array_json(json sequence, float**tones, float* len_tones);
	void set_params(int Fs, int n_ch, int n_total_stims, int frames_per_buffer);
	int n_ch;
	int Fs;
	int frames_per_buffer;
	float time;
	float** data;
	int* trig;
	int length;
};

AudioHandler::AudioHandler(){
	
}

AudioHandler::~AudioHandler(){

}

void AudioHandler::set_params(int Fs, int n_ch, int n_total_stims, int frames_per_buffer){
	std::cout << n_total_stims << std::endl;
	std::cout << n_ch << std::endl;
	std::cout << Fs << std::endl;
	this->n_ch = n_ch;
	this->Fs = Fs;
	this->N = n_total_stims;
	this->frames_per_buffer = frames_per_buffer;
	std::cout << this->N << std::endl;
	std::cout << this->n_ch << std::endl;
	std::cout << this->Fs << std::endl;
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

void AudioHandler::gen_array_json(json sequence, float**tones, float* len_tones){
	float* t = new float[this->N];
	int* ch = new int[this->N];
	int* stim = new int[this->N];
	int* trig_json = new int[this->N];
	
	int idx;

	std::cout << sequence << std::endl;
	
	for (int i = 0; i < this->N; i++) {
		std::cout << sequence[i] << std::endl;
		t[i] = sequence[i][0];
		ch[i] = sequence[i][1];
		stim[i] = sequence[i][2];
		trig_json[i] = sequence[i][3];
	}
	
	int Fs = this->Fs;
	int n_ch = this->n_ch;
	

	//float** y_mat = new float* [n_ch];
	float** y_mat = (float**)malloc(sizeof(float*) * n_ch);

	int n_stim = imax(stim, this->N) + 1;
	this->time = abs_fmax(t, this->N) + abs_fmax(len_tones, n_stim);

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
		idx = (int)(t[i] * Fs);
		if (stim[i] != -1){
			add_stim(&(y_mat[ch[i]][idx]), tones[stim[i]], (int)(len_tones[stim[i]] * Fs));
		}
	}
	

	//--------------
	// trig

	//int* trig_arr = new int[n_samples];
	int* trig_arr = (int*)malloc(sizeof(int) * n_samples);
	for (int i = 0; i < n_samples; i++) {
		trig_arr[i] = 0;
	}

	for (int i = 0; i < N; i++) {
		idx = (int)(t[i] * Fs);
		trig_arr[idx] = trig_json[i];
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

void AudioHandler::gen_array_csv(float** tones, float* len_tones) {
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
		if (stim_csv[i] != -1){
			add_stim(&(y_mat[ch_csv[i]][idx]), tones[stim_csv[i]], (int)(len_tones[stim_csv[i]] * Fs));
		}
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

void prepare(json json_data, AudioHandler* audio_handler){

	//-----------------------------------------------------------
	// read wav file

	std::cout << json_data << std::endl;
	
	unsigned int n_channels;
	unsigned int sampleRate;
	unsigned int frames_per_buffer;
	unsigned int n_total_stims;
	unsigned int n_stim;
	json files;	
	json sequence;
	
	n_channels = json_data["n_channels"];
	sampleRate = json_data["sample_rate"];
	frames_per_buffer = json_data["frames_per_buffer"];
	sequence = json_data["sequence"];
	files = json_data["files"];
		

	std::cout << n_channels << std::endl;
	std::cout << sampleRate << std::endl;
	std::cout << frames_per_buffer << std::endl;
	
	std::cout << sequence << std::endl;
	std::cout << files << std::endl;
	
	
	n_stim = files.size();
	n_total_stims = sequence.size();

	//std::ifstream ifs;
	//ifs.open(audio_files_csv);
	//char buf[512];
	//ifs.getline(buf, sizeof(buf));
	//int n_stim = (int)atof(buf) - 1;

	drwav_uint64* n_samples = new drwav_uint64[n_stim];
	float* len_tones = new float[n_stim];
	float** wav_data = new float*[n_stim];
	char filename[n_stim][1024];

	for (int i = 0; i < n_stim; i++) {
		strcpy(filename[i], files[i].get<std::string>().c_str());
	}

	for (int i = 0; i < n_stim; i++) {
		cout << "filename " << i << " : " << filename[i] << endl;
		wav_data[i] = drwav_open_file_and_read_pcm_frames_f32(filename[i], &n_channels, &sampleRate, &n_samples[i], NULL);
	}

	for (int i = 0; i < n_stim; i++){
		len_tones[i] = (float)n_samples[i] / sampleRate;
	}

	//AudioHandler audio_handler;
	
	audio_handler->set_params(sampleRate, n_channels, n_total_stims, frames_per_buffer);
	audio_handler->gen_array_json(sequence, wav_data, len_tones);

	std::cout << "audio data matrix was created." << std::endl;
}

void play(AudioHandler* audio_handler){

#if ENTRIG == 1
#if TRIG_DEV == 1
	char* bb_port = argv[5];
	cout << "ButtonBox Port : " << bb_port << endl;
#endif
#if TRIG_DEV == 0
	char* ni_port = argv[5];
	cout << "NI-DAQ Port : " << ni_port << endl;
#endif
#if TRIG_DEV == 2
	const char *lsl_name = "scab-c_marker";
	//char* ni_port = argv[5];
	
	lsl::stream_info info(lsl_name, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_string, "id23443");
	
	cout << "LSL marker stream name : " << lsl_name << endl;
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
#if TRIG_DEV == 2
	int trig = 0;	
	lsl::stream_outlet outlet(info);
	std::string mrk = "";
#endif
#endif
	
	//-------------------------------------------------------------------------------------------------
	// Start

	PaStreamParameters outParam;
	PaStream* stream;
	PaError err;
	padata data;

	int n_ch = audio_handler->n_ch;
	int Fs = audio_handler->Fs;
	int frames_per_buffer = audio_handler->frames_per_buffer;
	cout << "n_ch : " << n_ch << endl;
	cout << "time : " << audio_handler->time << endl;
	
	data.n_ch = n_ch;
	data.y = audio_handler->data;
	data.trig = audio_handler->trig;

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
	while (((double)(now.QuadPart - start.QuadPart) / clock.QuadPart) <= audio_handler->time) {

#if ENTRIG == 1
		if (data.current_trig != 0) {
			trig = data.current_trig;
#if TRIG_DEV == 0
			DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
#endif
#if TRIG_DEV == 1
			bb.sendMarker(trig);
#endif
# if TRIG_DEV == 2
			mrk = to_string(trig);
			outlet.push_sample(&mrk);
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
}

int main(int argc, char *argv[]) {
	
	//https://tecsingularity.com/winsock/winsock2/
	
	int port_number = 65500;
	
	WSADATA wsa_data;

	// initialize
	if (WSAStartup(MAKEWORD(2, 0), &wsa_data) != 0) {
		std::cerr << "Initializing Winsock(WSAStartup) was failed." << std::endl;
	}
	
	int src_socket;

	struct sockaddr_in src_addr;
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.sin_port = htons(port_number);
	src_addr.sin_family = AF_INET;
	src_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// AF_INET		: IPv4 IP
	// SOCK_STREAM	: TCP
	src_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	bind(src_socket, (struct sockaddr *) &src_addr, sizeof(src_addr));
	
	int dst_socket;
	struct sockaddr_in dst_addr;
	int dst_addr_size = sizeof(dst_addr);
	
	std::cout << "socket was created." << std::endl;
	
	listen(src_socket, 1);
	
	char recv_buf1[100000], recv_buf2[4096];
	char send_buf[4096];
	char msg[256];

	while (1) {

		std::cout << "waiting for connection..." << std::endl;

		// クライアントからの接続を受信する
		dst_socket = accept(src_socket, (struct sockaddr *) &dst_addr, &dst_addr_size);

		std::cout << "connected." << std::endl;

		// 接続後の処理
		while (1) {

			int status;

			//パケットの受信(recvは成功すると受信したデータのバイト数を返却。切断で0、失敗で-1が返却される
			int recv1_result = recv(dst_socket, recv_buf1, sizeof(char) * 100000, 0);
			if (recv1_result == 0 || recv1_result == -1) {
				status = closesocket(dst_socket); break;
			}
			json json_data = json::parse(recv_buf1);
			std::cout << "recieved : " << json_data << std::endl;

			AudioHandler* audio_handler;
			audio_handler = new AudioHandler[1];
			
			prepare(json_data, audio_handler);
			play(audio_handler);
			
			std::cout << "end" << std::endl;

			// 結果を格納したパケットの送信
			//msg = 'end';
			strcpy(msg, "end");
			send(dst_socket, msg, sizeof(char) * 256, 0);
		}
	}
	
	// WinSockの終了処理
	WSACleanup();


	return 0;
}