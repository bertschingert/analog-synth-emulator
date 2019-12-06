#include "audio.h"
#include "synthesizer.h"
#include "RtAudio.h"

Synthesizer g;
LowFrequencyOscillator l;
ADSREnvelope e;
Filter f;
Distortion d;

std::ofstream out_log;
std::queue<float> debug_samp;

RtAudio dac;

std::queue<std::vector<unsigned char>> midi_q;
std::mutex mtx_q;
std::condition_variable cv_q;
bool done_writing;

// vector to remember the notes that are currently held down
std::vector<unsigned char> note_stack;
unsigned char current_note;

// wavetables
std::vector<float> wave_table_saw;
std::vector<float> wave_table_sine;
std::vector<float> wave_table_tri;
std::vector<float> wave_table_square;

int callback(void *outputBuffer, void * /*inputBuffer*/,
                  unsigned int nBufferFrames, double /*streamTime*/,
                  RtAudioStreamStatus status, void * /*data*/) {
    MY_TYPE *buffer = (MY_TYPE *)outputBuffer;
    if (status) std::cout << "Stream underflow detected!" << std::endl;

    float val = 0;        // the output sample
    float val_lfo = 0;    // data point from LFO
    float steps = 0;      // used for bitcrusher
    float index = 0;      // used for bitcrusher

    for (uint32_t i = 0; i < nBufferFrames; i++) {

      // avoid excessively high frequency
       if (g.freq > 20000) {
          // std::cout << "was " << g.freq << std::endl;
          g.freq = 20000;
          g.phase_incr = g.freq * phase_factor;
      }

      // LFO OPERATION BELOW
      if(i%16 == 0) {            // control period: 16 * sample period
          if ((g.freq != 0) && l.enabled) {     // only use LFO if a note is playing
              if (l.phase > TABLE_SIZE) l.phase = fmod(l.phase, TABLE_SIZE);
              if (l.phase < 0) l.phase = 0;
              switch (l.osc_type) {
              case 0:
                  val_lfo = wave_table_sine.at((int) l.phase);
                  break;
              case 1:
                  val_lfo = wave_table_saw.at((int) l.phase);
                  break;
              case 2:
                  val_lfo = wave_table_tri.at((int) l.phase);
                  break;
              case 3:
                  val_lfo = wave_table_square.at((int) l.phase);
                  break;
              }
              l.phase += l.phase_incr;
              if (l.phase > TABLE_SIZE) l.phase = fmod(l.phase, TABLE_SIZE);

              if (l.choice == 0) { // LFO affects Frequency
                  l.o_freq = g.freq * ( pow( pow(CENT, l.amount), val_lfo) );
                  l.o_phase_incr = l.o_freq * phase_factor;
              }
              else if (l.choice == 1) { // LFO affects Ratio
                  l.o_ratio = ((l.amount / 1000.0) * val_lfo) + g.osc_ratio;
                  if (l.o_ratio > 1)
                      l.o_ratio = 1;
                  if (l.o_ratio < 0)
                      l.o_ratio = 0;
              }
              else if (l.choice == 2) { // LFO affects Filter
                  l.f_cutoff = (l.amount * val_lfo) + f.cutoff;
                  if (l.f_cutoff < 1) l.f_cutoff = 1;
                  initialize_filter();
              }
              else if (l.choice == 3) { // LFO affects Distortion
                  if (d.type == 0) { // for overdrive
                      l.o_amount = (l.amount / 100.0) * val_lfo + d.o_amount;
                      if (l.o_amount < 1) l.o_amount = 1;
                      if (l.o_amount > 15) l.o_amount = 15;
                  }
                  else if (d.type == 1) { // for bitcrushing
                      l.b_amount = (l.amount / 100) * val_lfo + d.b_amount;
                      if (l.b_amount < 2) l.b_amount = 2;
                      if (l.b_amount > 16) l.b_amount = 16;
                  }
              }
          } else {
              val_lfo = 0;
              l.o_ratio = 0;
              l.o_freq = 0;
              l.o_phase_incr = 0;
          }
      } // end control period

    val = g.get_sample();

    // DISTORTION
    if (d.enabled) {
        if (l.enabled && l.choice == 3) { // if the LFO is set to distortion
            if (d.type == 0) { // Overdrive
                val *= l.o_amount;
                if (val > 1.0) val = 1.0;
                if (val < -1.0) val = -1.0;
            }
            else if (d.type == 1) { // Bitcrusher
                steps = pow(2, l.b_amount) / 2.0;
                index = 0;
                if (val > 0){
                    while (val > index / steps) {
                        index += steps / l.b_amount;
                    }
                    val = index / steps;
                }
                else {
                    while (val < -1 * index / steps) {
                        index += steps / l.b_amount;
                    }
                    val = (-1 * index) / steps;
                }
            }
        } // below if LFO not affecting distortion
        else {
            if (d.type == 0) { // Overdrive
                val *= d.o_amount;
                if ( val > 1.0) val = 1.0;
                if (val < -1.0) val = -1.0;
            }
            else if (d.type == 1) { // Bitcrusher
                steps = pow(2, d.b_amount) / 2.0;
                index = 0;
                if (val > 0){
                    while (val > index / steps) {
                        index += steps / d.b_amount;
                    }
                    val = index / steps;
                }
                else {
                    while (val < -1 * index / steps) {
                        index += steps / d.b_amount;
                    }
                    val = (-1 * index) / steps;
                }
            }
        }
    }

    // envelope
    if (e.state == 'a') {
        e.cur_amp = (float) g.samp * (1 - e.starting_amp) / e.a_s + e.starting_amp;
        if (g.samp >= e.a_s) {
            g.samp = -1;
            e.state = 'd';
        }
    }
    else if (e.state == 'd') {
        e.cur_amp = 1 - ((float) g.samp / e.d_s) * (1 - e.sustain);
        if (g.samp >= e.d_s) {
            g.samp = -1;
            e.state = 's';
        }
    }
    else if (e.state == 's') {
        e.cur_amp = e.sustain;
        // wait for note off to change state to 'r'
    }
    else if (e.state == 'r') {
        e.cur_amp = e.starting_amp + ((-1 * (float) g.samp) / e.r_s) * e.starting_amp;
        if (g.samp >= e.r_s) {
            e.cur_amp = 0;
            g.freq = 0;
            e.state = 'z';
        }
    }
    if (e.cur_amp > 1) e.cur_amp = 1;
    ++g.samp;

    // apply the envelope
    val *= e.cur_amp;

    // FILTER
    f.x2 = f.x1;
    f.x1 = val;

    if (f.filter_enabled) {
        val = f.a0 * val + f.a1 * f.x1 + f.a2 * f.x2 - f.b1 * f.y1 - f.b2 * f.y2;
    }

    f.y2 = f.y1;
    f.y1 = val;

    // we never want the absolute value of a sample to exceed 1
    for (auto num : f.values) {      // f.values contains pointers to each of
        if (*num > 1) *num = 1;     // the sample values used in the filter
        if (*num < -1) *num = -1;
    }

    // gain
    val *= g.output_volume;
    if ( val > 1.0) val = 1.0;
    if (val < -1.0) val = -1.0;

    // SEND SAMPLE ==> DAC
    if (std::isnan(val)) out_log << "invalid sample" << std::endl;
    *buffer++ = val;
    // uncomment below for debug
    // this queue keeps a record of the 100 most recent samples
    // debug_samp.push(val);
    // debug_samp.pop();

    if (l.enabled && (l.choice == 0)) // if LFO affecting frequency
        g.phase += l.o_phase_incr;
    else
        g.phase += g.phase_incr;

    // make sure the phase is in bound
    if (g.phase > TABLE_SIZE) g.phase = fmod(g.phase, TABLE_SIZE);
  }
  return 0;
}

void watch_midi(){
    int pbend_pre = 64;
    int pbend_cur = 0;
    // stay in this loop until program is over
    while (!done) {
        // the queue of midi messages is accessed by this thread (audio)
        // and the midi thread. so we need to acquire a lock to use it.

        std::unique_lock<std::mutex> my_lock(mtx_q);
        cv_q.wait(my_lock, [] { return done_writing; } );

        // once we've been notified that the queue has a message, we can move on
        if (midi_q.size() > 0) {
            std::vector<unsigned char> message = midi_q.front();
            midi_q.pop();
            unsigned char status = message.at(0);

            if (status > 0x89 && status < 0xa0) {           // NOTE ON
                current_note = message.at(1);
                // add the note to our stack
                note_stack.push_back(current_note);
                // start outputting this note
                g.samp = 0;
                e.state = 'a';
                e.starting_amp = e.cur_amp;
                g.freq = midi_to_freq(current_note);
                g.phase_incr = phase_factor * g.freq;
            } else if (status > 0x79 && status < 0x90) {    // NOTE OFF
                current_note = message.at(1);
                // take the note out of our stack
                for( uint i = 0; i < note_stack.size(); ++i) {
                    if (note_stack.at(i) == current_note){
                        note_stack.erase(note_stack.begin() + i);
                        break;
                    }
                }
                if (note_stack.size() == 0) { // if no notes are held down
                    e.state = 'r';
                    e.starting_amp = e.cur_amp;
                    g.samp = 0;
                } else { // if we have another note still held down, then play it
                    e.state = 'a';
                    e.starting_amp = e.cur_amp;
                    g.samp = 0;
                    g.freq = midi_to_freq(note_stack.back());
                    g.phase_incr = phase_factor * g.freq;
                }
            } else if (status > 0xdf && status < 0xf0) { // PITCH BEND
                // range 0 - 127, with 64 being no difference in pitch
                if (g.freq > 0) { // only apply pitch bend if we're already playing a note
                    float amount;
                    pbend_cur = message.at(2);      // data 2 holds the info we want
                    amount = pbend_cur - pbend_pre; // will be +1 if moving up, or -1 if moving down
                    if (pbend_cur > 63)
                        amount *= g.pbend_range / 63;
                    else
                        amount *= g.pbend_range / 64;
                    g.freq *= pow(CENT, amount);
                    g.phase_incr = g.freq * phase_factor;
                    pbend_pre = pbend_cur;
                }
            }
        }
        my_lock.unlock();
        // end of critical section
    }
}

void initialize_wavetables(){
    for (int i = 0; i < TABLE_SIZE; i++){
        if (i < 256) // TRIANGLE
            wave_table_tri.push_back( (i / 255.0) );
        else if (i < 768)
            wave_table_tri.push_back( (512 - i) / 256.0 );
        else
            wave_table_tri.push_back( ((i - 768) / 256.0) - 1);

        if (i < 512) {
            wave_table_saw.push_back(i / 511.0);    // SAW
            wave_table_square.push_back(-1);        // SQUARE
        }
        else {
            wave_table_saw.push_back( (i - 512) / 511.0 - 1.0); // SAW
            wave_table_square.push_back(1);                     // SQUARE
        }
        // SINE
        wave_table_sine.push_back(sin(TWOPI * (i / 1023.0)));
    }
}

void print_audio_device_info() {
    int devices = dac.getDeviceCount();
    struct RtAudio::DeviceInfo info;
    for (int i=1; i<=devices; i++) {

      try {
          info = dac.getDeviceInfo(i);
      }
      catch (RtAudioError &error) {
          error.printMessage();
      }
      std::cout << "device = " << i;
      std::cout << ": name = " << info.name << " default: " << (info.isDefaultOutput ? "yes" : "no") << std::endl;
    }

    int def = dac.getDefaultOutputDevice();
    info = dac.getDeviceInfo(def);
    std::cout << "default dev: " << def << " name: " << info.name << std::endl;
}

void initialize_audio() {
    // hardcoding the device for now... may need to make this chooseable by user
    unsigned int device = 6;
    unsigned int channels = 1;
    unsigned int bufferFrames = 512;
    unsigned int offset = 0;

    initialize_wavetables();

    RtAudio::StreamParameters oParams;
    RtAudio::StreamOptions options;
    double *rtdata = (double *)calloc(channels, sizeof(double));

    // out_log is used to keep track of debugging information
    out_log.open("log.txt");

    if (dac.getDeviceCount() < 1) {
        std::cout << "\nNo audio devices found!\n";
        exit(1);
    }

    // Let RtAudio print messages to stderr.
    dac.showWarnings(true);

    // Set our stream parameters for output only.
    oParams.deviceId = device;
    oParams.nChannels = channels;
    oParams.firstChannel = offset;

    if (device == 0) oParams.deviceId = dac.getDefaultOutputDevice();

    options.flags = RTAUDIO_HOG_DEVICE;
    options.flags |= RTAUDIO_SCHEDULE_REALTIME;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    try {
        dac.openStream(&oParams, NULL, RTAUDIO_FLOAT32, SR, &bufferFrames, &callback,
                     (void *)rtdata, &options, &errorCallback);
        dac.startStream();
    } catch (RtAudioError &e) {
        e.printMessage();
    }
    watch_midi();
}

void errorCallback(RtAudioError::Type type, const std::string &errorText) {
    if (type == RtAudioError::WARNING)
      std::cerr << errorText << std::endl;
    else if (type != RtAudioError::WARNING)
      throw(RtAudioError(errorText, type));
}

void audio_cleanup() {
    try {
        if (dac.isStreamOpen()){
          // dac.stopStream();
          dac.abortStream();
        }
    } catch (RtAudioError &e) {
        e.printMessage();
    }
}

float midi_to_freq(unsigned char note){
    int n = static_cast<int> (note);
    float f;
    f = (440.0 / 32.0) * pow(2, ((n - 9.0) / 12.0));
    return f;
}

void initialize_filter() {
    // determine f.C coefficient based on the cutoff
    // the cutoff may be modulated by the LFO
    if (l.enabled && l.choice == 2) {
        if (f.filter_type == 0) // low pass
            f.C = 1 / tan((M_PI * l.f_cutoff) / SR);
        else if (f.filter_type == 1) // high pass
            f.C = tan((M_PI * l.f_cutoff) / SR);
        else // for resonant
            f.C = 2 * f.resonance * cos((2 * M_PI * l.f_cutoff) / SR);
    } else {
        if (f.filter_type == 0) // low pass
            f.C = 1 / tan((M_PI * f.cutoff) / SR);
        else if (f.filter_type == 1) // high pass
            f.C = tan((M_PI * f.cutoff) / SR);
        else // for resonant
            f.C = 2 * f.resonance * cos((2 * M_PI * f.cutoff) / SR);
    }

    if (f.filter_type == 0) {       // LOW PASS
        f.D = pow(f.C, 2) + f.C * sqrt(2.0) + 1;
        f.a0 = 1 / f.D;
        f.a1 = 2 / f.D;
        f.a2 = 1 / f.D;
        f.b1 = (2 * (1 - pow(f.C, 2))) / f.D;
        f.b2 = (pow(f.C, 2) - f.C * sqrt(2.0) + 1) / f.D;
    }
    else if (f.filter_type == 1) {  // HI PASS
        f.D = pow(f.C, 2) + f.C * sqrt(2.0) + 1;
        f.a0 = 1 / f.D;
        f.a1 = -2 / f.D;
        f.a2 = 1 / f.D;
        f.b1 = (2 * (pow(f.C, 2) - 1)) / f.D;
        f.b2 = (pow(f.C, 2) - f.C * sqrt(2.0) + 1) / f.D;
    }
    else if (f.filter_type == 2) {  // RESONANT
        f.a0 = 0.5 - (pow(f.resonance, 2) / 2);
        f.a1 = 0;
        f.a2 = f.C;
        f.b1 = f.C * -1;
        f.b2 = pow(f.resonance, 2);
    }
}
