#include <iostream>
#include <cstdlib>
#include <map>
#include <math.h>

#include <vector>
#include <queue>

#include <mutex>
#include <condition_variable>

#include "midi.h"


RtMidiIn *midiin = 0;
std::thread midi_thread(midi_watcher);

std::mutex mtx_port_change;
std::condition_variable cv_port_change;
bool port_chosen = false;

void mycallback(double deltatime, std::vector<unsigned char> *message,
                void * /*userData*/) {
//  int nBytes = message->size();
  // print the midi message to stdout
//  for (int i = 0; i < nBytes; i++) {
//    if (i == 0)  // the status byte must be printed in hex
//      std::cout << std::hex;
//    else
//      std::cout << std::dec;
//    std::cout << '\t' << static_cast<uint16_t>(message->at(i));
//    if (i == nBytes - 1) std::cout << std::endl;
//  }
    // acquire the lock to the shared midi message queue
    std::unique_lock<std::mutex> q_lock(mtx_q);
    done_writing = false;

    midi_q.push(*message);

    done_writing = true;
    cv_q.notify_one();
    // end of critical section
}

std::vector<std::string> setup_rtmidi() {
    done_writing = true;
    // put the list of midi devices in a vector
    // so that we can select the device from the ui
    std::vector<std::string> midi_devices;
    std::string portName;

    try {
        midiin = new RtMidiIn();

        unsigned int i = 0, nPorts = midiin->getPortCount();

        if (nPorts == 0) {
          std::cout << "No input ports available!" << std::endl;
          return midi_devices;
        }

        for (i = 0; i < nPorts; i++) {
          portName = midiin->getPortName(i);
          midi_devices.push_back(portName);
        }
      }
      catch (RtMidiError &error) {
        error.printMessage();
      }

    return midi_devices;
}

void midi_watcher(){
    std::unique_lock<std::mutex> my_lock(mtx_port_change);
    // wait until signaled that the port is open
    cv_port_change.wait(my_lock, [] {return port_chosen; });
    midiin->setCallback(&mycallback);

    midiin->ignoreTypes(false, false, false);
    my_lock.unlock();
}

bool chooseMidiPort(int midi_device) {
    std::unique_lock<std::mutex> my_lock(mtx_port_change);

    port_chosen = false;

    if ( midiin->isPortOpen() ) {
        midiin->closePort();
    }

    midiin->openPort(midi_device);
    port_chosen = true;
    cv_port_change.notify_one();
    // end of critical section
    return midiin->isPortOpen();

}

void midi_cleanup() {
    delete midiin;
    midi_thread.join();
}
