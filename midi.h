#ifndef MIDI_H
#define MIDI_H

#include "RtMidi.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>


extern std::queue<std::vector<unsigned char>> midi_q;
extern std::mutex mtx_q;
extern std::condition_variable cv_q;
extern bool done_writing;

void mycallback(double deltatime, std::vector<unsigned char> *message);
bool chooseMidiPort(int midi_device);

std::vector<std::string> setup_rtmidi();

void midi_cleanup();

void midi_watcher();

#endif // MIDI_H
