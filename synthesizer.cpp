#include "synthesizer.h"

float Synthesizer::get_sample() {
    float val = 0;        // the output sample
    float val_1 = 0;      // data point from osc 1
    float val_2 = 0;      // data point from osc 2

    if (this->phase > TABLE_SIZE) this->phase = fmod(this->phase, TABLE_SIZE);
          if (this->phase < 0) this->phase = 0;
          switch (this->osc1_type) {
          case 0:
              val_1 = wave_table_sine.at((int) this->phase);
              break;
          case 1:
              val_1 = wave_table_saw.at((int) this->phase);
              break;
          case 2:
              val_1 = wave_table_tri.at((int) this->phase);
              break;
          case 3:
              val_1 = wave_table_square.at((int) this->phase);
              break;
          }
          switch (this->osc2_type) {
          case 0:
              val_2 = wave_table_sine.at((int) this->phase);
              break;
          case 1:
              val_2 = wave_table_saw.at((int) this->phase);
              break;
          case 2:
              val_2 = wave_table_tri.at((int) this->phase);
              break;
          case 3:
              val_2 = wave_table_square.at((int) this->phase);
              break;
          }

        // RATIO
        if(l.enabled && l.choice == 1) {
            val = (val_1 *  l.o_ratio) + (val_2 * (1 - l.o_ratio));
        } else {
            val = (val_1 *  this->osc_ratio) + (val_2 * (1 - this->osc_ratio));
        }
    return val;
}
