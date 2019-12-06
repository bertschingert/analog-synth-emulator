#ifndef AUDIO_H
#define AUDIO_H

#include "midi.h"
#include "RtAudio.h"

#include <cmath>
#include <fstream>

// RtAudio globals
#define SCALE 1.0
typedef float MY_TYPE;

// Digital Audio constants
const int SR = 44100;  // sample rate
const float T = 1.0 / SR;  // sample period
const float TWOPI = 2 * M_PI;
const float TWOPI_T = TWOPI * T; // radians per sample
const double CENT = 1.000577789506553;

const int TABLE_SIZE = 1024;
const float phase_factor = 1024.0/SR; // 1024 is table length


extern bool done;
extern std::queue<float> debug_samp;
extern std::ofstream out_log;

void errorCallback(RtAudioError::Type type, const std::string &errorText);
int callback(void *outputBuffer, void * /*inputBuffer*/,
                  unsigned int nBufferFrames, double /*streamTime*/,
                  RtAudioStreamStatus status, void *data);

float midi_to_freq(unsigned char);
void initialize_filter();
void initialize_audio();
void audio_cleanup();

#endif
