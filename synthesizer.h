#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include "audio.h"

class ADSREnvelope
{
public:
    float attack;
    float decay;
    float sustain;
    float release;
    char state;

    int a_s;
    int d_s;
    int r_s;
    float cur_amp;
    float starting_amp;

    ADSREnvelope() : attack(0.01),
        decay(0.1),
        sustain(1),
        release(0.01),
        state('z'),
        a_s((int)(attack * SR)),
        d_s((int)(decay * SR)),
        r_s((int)(release * SR)),
        cur_amp(0),
        starting_amp(0) {}
};

class LowFrequencyOscillator
{
public:
    float freq;
    int choice;
    bool enabled;
    float phase;
    float phase_incr;
    int osc_type;

    float o_freq;
    float o_phase_incr;
    float o_ratio;

    float f_cutoff;

    int b_amount;
    float o_amount;

    int amount;

    LowFrequencyOscillator() : freq(5),
        choice(0),
        enabled(true),
        phase(0),
        phase_incr(freq * phase_factor * 16),
        osc_type(0),
        o_freq(0),
        o_phase_incr(0),
        o_ratio(0),
        f_cutoff(440),
        b_amount(0),
        o_amount(0),
        amount(50) {}
};

class Filter {
public:
    int filter_type;
    bool filter_enabled;
    float cutoff;
    float resonance;
    float bandwidth;

    float x1;
    float x2;
    float y1;
    float y2;

    float C;
    float D;
    float a0;
    float a1;
    float a2;
    float b1;
    float b2;

    std::vector<float *> values;

    Filter() : filter_type(0),
        filter_enabled(false),
        cutoff(440),
        resonance(0),
        bandwidth(0),
        x1(0),
        x2(0),
        y1(0),
        y2(0),
        C(0),
        D(0),
        a0(0),
        a1(0),
        a2(0),
        b1(0),
        b2(0) {}
};

class Distortion {
public:
    bool enabled;
    int type;
    int b_amount;
    float o_amount;

    Distortion() : enabled(false),
        type(0),
        b_amount(16),
        o_amount(1) {}
};

class Synthesizer
{
public:
    float freq;
    float osc_ratio;
    int osc1_type;
    int osc2_type;

    int samp; // the current sample number
    float phase; // current phase
    float phase_incr; // calculate based on value of freq
    float output_volume;

    int midi_device;
    int pbend_range;

    Synthesizer() : freq(0),
        osc_ratio(.5),
        osc1_type(0),
        osc2_type(0),
        samp(0),
        phase(0),
        phase_incr(freq * TWOPI_T),
        output_volume(.75),
        midi_device(-1),
        pbend_range(100) {}
};

extern Synthesizer g;
extern LowFrequencyOscillator l;
extern ADSREnvelope e;
extern Filter f;
extern Distortion d;

#endif // SYNTHESIZER_H
