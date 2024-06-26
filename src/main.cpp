//---------------------------------------------------------------------
//	scab
//---------------------------------------------------------------------
//#pragma comment(lib, "portaudio_x64.lib")
//#define _GLIBCXX_USE_CXX11_ABI 0
#define DR_WAV_IMPLEMENTATION
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <math.h>
#include <bitset>
#include <netinet/in.h>

#include <unistd.h>
#include "portaudio.h"
#include "dr_wav.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define PORT 49152
#define HEADER_LENGTH 64
#define BUFFER_LENGTH 122880

//#define ENTRIG
//#define TRIG_DEV
// TRIG_DEV
// 0 : NIDAQ
// 1 : ButtonBox
// 2 : LSL

//using namespace std;

//#define ENTRIG 1
//#define TRIG_DEV 0

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

int intToCharArray(char* buffer, int in){
	// for little endian

	std::bitset<HEADER_LENGTH> data_bit(in);
	int cnt = 0;
	for (int i = (HEADER_LENGTH-1); i >= 0; i--){
		std::bitset<HEADER_LENGTH> val = data_bit >> (i*8);
		val &= 0xff;
		buffer[i] = val.to_ulong();
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
	/*
	std::cout << n_total_stims << std::endl;
	std::cout << n_ch << std::endl;
	std::cout << Fs << std::endl;
	*/
	this->n_ch = n_ch;
	this->Fs = Fs;
	this->N = n_total_stims;
	this->frames_per_buffer = frames_per_buffer;
	/*
	std::cout << this->N << std::endl;
	std::cout << this->n_ch << std::endl;
	std::cout << this->Fs << std::endl;
	*/
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

	/*
	std::cout << sequence << std::endl;
	*/

	for (int i = 0; i < this->N; i++) {
		//std::cout << sequence[i] << std::endl;
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

void sleep(int time_millisec){
	int64_t time_nanosec = (int64_t) time_millisec*1000000.0; // convert from millisec to nanosec
	int64_t start, end;
    start = std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
    end = std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
	
	while ((end-start) <= time_nanosec) {
    	end = std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	
}

/*
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
*/

void prepare(json json_data, AudioHandler* audio_handler){

	//-----------------------------------------------------------
	// read wav file

	//std::cout << json_data << std::endl;
	
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
		
	
	/*
	std::cout << n_channels << std::endl;
	std::cout << sampleRate << std::endl;
	std::cout << frames_per_buffer << std::endl;

	std::cout << sequence << std::endl;
	std::cout << files << std::endl;
	*/
	
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

	unsigned int wav_n_channels;
	unsigned int wav_sampleRate;
	for (int i = 0; i < n_stim; i++) {
		cout << "filename " << i << " : " << filename[i] << endl;
		wav_data[i] = drwav_open_file_and_read_pcm_frames_f32(filename[i], &wav_n_channels, &wav_sampleRate, &n_samples[i], NULL);
	}

	for (int i = 0; i < n_stim; i++){
		len_tones[i] = (float)n_samples[i] / sampleRate;
	}

	//AudioHandler audio_handler;
	
	audio_handler->set_params(sampleRate, n_channels, n_total_stims, frames_per_buffer);
	audio_handler->gen_array_json(sequence, wav_data, len_tones);

	std::cout << "audio data matrix was created." << std::endl;
}

//lsl::stream_outlet outlet(info);
#if ENTRIG == 1 && TRIG_DEV == 2
void play(json json_data, AudioHandler* audio_handler, lsl::stream_outlet* outlet){
#else
void play(json json_data, AudioHandler* audio_handler){
#endif
	
	std::cout << "play was called" << std::endl;

#if ENTRIG == 1
#if TRIG_DEV == 1
	char* bb_port = argv[5];
	cout << "ButtonBox Port : " << bb_port << endl;
#endif
#if TRIG_DEV == 0
	//char* ni_port = argv[5];
	char ni_port[4096];
	//json_data["ni_port"];
	std::cout << "here" << std::endl;
	strcpy(ni_port, json_data["ni_port"].get<std::string>().c_str());
	std::cout << "NI-DAQ Port : " << ni_port << std::endl;
#endif
#if TRIG_DEV == 2
/*
	const char *lsl_name = "scab-c_marker";
	//char* ni_port = argv[5];
	
	lsl::stream_info info(lsl_name, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_string, "id23443");
	
	std::cout << "LSL marker stream name : " << lsl_name << std::endl;
*/
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
	//lsl::stream_outlet outlet(info);
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
	std::cout << "n_ch : " << n_ch << std::endl;
	std::cout << "time : " << audio_handler->time << std::endl;
	std::cout << "frames_per_buffer : " << frames_per_buffer << std::endl;
	
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
	std::cout << "API : " << Pa_GetHostApiInfo(Pa_GetDefaultHostApi())->name << std::endl;
	if (strcmp(Pa_GetHostApiInfo(Pa_GetDefaultHostApi())->name, "ASIO") != 0) {
		std::cout << "Warning : This session is NOT running on ASIO." << std::endl;
	}

	//-----------------------------------------------------------
	// initialize query performance counter
    int64_t start, now;

	start = std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
	now = std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
	
	//LARGE_INTEGER start, now, clock;
	//QueryPerformanceFrequency(&clock);
	//-----------------------------------------------------------
	//QueryPerformanceCounterSleep(1000, clock);
	
	sleep(1000);

	//-----------------------------------------------------------
	Pa_StartStream(stream);  // Start Port Audio
	//-----------------------------------------------------------

	std::cout << "Started" << std::endl;

	//QueryPerformanceCounter(&start);
	//QueryPerformanceCounter(&now);
	while ((now - start)/1000000000.0 <= audio_handler->time) {

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
			mrk = std::to_string(trig);
			outlet->push_sample(&mrk);
#endif
			trig = 0;
			data.current_trig = 0;
			//QueryPerformanceCounterSleep(5, clock);
			sleep(5);
#if TRIG_DEV == 0
			DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &trig, &written, NULL);
#endif
#if TRIG_DEV == 1
			bb.sendMarker(trig);
#endif
		}
#endif
		//QueryPerformanceCounter(&now);
		now = std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	//QueryPerformanceCounterSleep(1000, clock);
	sleep(1000);

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
	
    int sockfd, new_sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // assign address
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
	std::cout << "socket was created." << std::endl;

	char recv_buf[BUFFER_LENGTH];
	char send_buf[BUFFER_LENGTH];
	char buffer[HEADER_LENGTH];
	

#if ENTRIG == 1
#if TRIG_DEV == 2
	const char *lsl_name = "scab-c";
	//char* ni_port = argv[5];
	
	lsl::stream_info info(lsl_name, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_string, "id23443");
	
	lsl::stream_outlet outlet(info);
	std::cout << "LSL marker stream name : " << lsl_name << std::endl;

#endif
#endif

	while (1) {

		std::cout << "waiting for connection..., port: " << PORT << std::endl;

		listen(sockfd, 5);
		clilen = sizeof(cli_addr);
		new_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (new_sockfd < 0) {
			perror("ERROR on accept");
			break;
			//exit(1);
		}

		std::cout << "connected." << std::endl;

		while (1) {

			//int status;

			// == Recieve ==
			memset(recv_buf, 0, BUFFER_LENGTH);
			n = recv(new_sockfd, recv_buf, BUFFER_LENGTH, 0);
			if (n < 0) {
				perror("ERROR reading from socket");
				break;
				//exit(1);
			}
			

			// convert little endian data to int
			unsigned int length;
			std::memcpy(&length, recv_buf, sizeof(unsigned int));
			
			if (length == 0){
				break;
			}
			std::cout << "recieved length : " << length << std::endl;

			memset(recv_buf, 0, length);
			n = recv(new_sockfd, recv_buf, length, 0);
			if (n < 0) {
				perror("ERROR reading from socket");
				break;
				//exit(1);
			}

			recv_buf[length] = 0;
			//json json_data = json::parse(recv_buf);
			//td::cout << json_data << std::endl;
			
			std::cout << recv_buf << std::endl;

			json json_data = json::parse(recv_buf);
			std::cout << "recieved : " << json_data << std::endl;

			// == Recieve end ==


			AudioHandler* audio_handler;
			audio_handler = new AudioHandler[1];
			
			prepare(json_data, audio_handler);


			#if ENTRIG == 1 && TRIG_DEV == 2
			play(json_data, audio_handler, &outlet);
			#else
			play(json_data, audio_handler);
			#endif

			
			// == Send ==
			json j;
			j["type"] = "info";
			j["info"] = "playback-finish";
			
			std::string s = j.dump();
			
			intToCharArray(buffer, s.size());

			n = send(new_sockfd, buffer, HEADER_LENGTH, 0);
			if (n < 0) {
				perror("ERROR writing to socket");
				break;
				//exit(1);
			}
			n = send(new_sockfd, s.c_str(), s.size(), 0);
			if (n < 0) {
				perror("ERROR writing to socket");
				break;
				//exit(1);
			}
			/*
			send(dst_socket, buffer, HEADER_LENGTH, 0);
			send(dst_socket, s.c_str(), s.size(), 0);
			*/
			
			// == Send end //
			
			std::cout << "playback finish" << std::endl;

			//strcpy(msg, "end");
			//send(dst_socket, msg, sizeof(char) * 256, 0);
		}
	}
	
	// WinSockの終了処理
	//WSACleanup();

    close(new_sockfd);
    close(sockfd);

	return 0;
}