#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "audio.h"
#include "midi.h"

bool done;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    done = false;
    ui->setupUi(this);
    initialize_filter();

    std::vector<std::string> midi_devices = setup_rtmidi();
    init_controls(midi_devices);
    f.values.push_back(&f.x1);
    f.values.push_back(&f.x2);
    f.values.push_back(&f.y1);
    f.values.push_back(&f.y2);
    for(int i = 0; i < 99; ++i) debug_samp.push(0);
}

MainWindow::~MainWindow()
{
    done = true;
    float s;
    while(!debug_samp.empty()){
        s = debug_samp.front();
        out_log << "debug " << s << std::endl;
        debug_samp.pop();
    }
    midi_cleanup();
    audio_cleanup();
    delete ui;
}

void MainWindow::init_controls(std::vector<std::string> midi_devices) {
    QString name;
    for (auto i : midi_devices){
        name = QString::fromStdString(i);
        ui->comboBox_midi->addItem(name);
    }
}
QString label;

// oscillators

void MainWindow::on_comboBox_osc1_currentIndexChanged(int index)
{
    g.osc1_type = index;
}

void MainWindow::on_comboBox_osc2_currentIndexChanged(int index)
{
    g.osc2_type = index;
}

void MainWindow::on_verticalSlider_oscRatio_valueChanged(int value)
{
    g.osc_ratio = value/100.0;
    label = QString::number(g.osc_ratio);
    ui->label_oscRatio->setText(label);
}

// envelope
// range of int value is 0 - 99

void MainWindow::on_verticalSlider_attack_valueChanged(int value)
{
    // attack range is 0.05 - 5 seconds
    e.attack = (value + 1) / 200.0;
    e.a_s = (int)(e.attack * SR);
    label = QString::number(e.attack);
    label += "s";
    ui->label_attack->setText(label);
}

void MainWindow::on_verticalSlider_decay_valueChanged(int value)
{
    // decay range is 0.05 - 2 seconds
    e.decay = (value + 1) / 50.0;
    e.d_s = (int)(e.decay * SR);
    label = QString::number(e.decay);
    label += "s";
    ui->label_decay->setText(label);
}

void MainWindow::on_verticalSlider_sustain_valueChanged(int value)
{
    // sustain range is 0 - 1 * output volume
    e.sustain = (value + 1) / 100.0;
    label = QString::number(e.sustain);
    ui->label_sustain->setText(label);
}

void MainWindow::on_verticalSlider_release_valueChanged(int value)
{
    e.release = (value + 1) / 500.0;
    e.r_s = (int)(e.release * SR);
    label = QString::number(e.release);
    label += "s";
    ui->label_release->setText(label);
}

// LFO

void MainWindow::on_pushButton_LFO_clicked()
{
    l.enabled = !l.enabled;
    if (l.enabled) label = "On"; else label = "Off";
    ui->pushButton_LFO->setText(label);
    initialize_filter();
}

void MainWindow::on_horizontalSlider_LFO_valueChanged(int value)
{
    l.freq = value/10.0;
    l.phase_incr = l.freq * phase_factor * 16;
    label = QString::number(l.freq);
    label += " Hz";
    ui->label_LFO->setText(label);
}

void MainWindow::on_comboBox_LFO_currentIndexChanged(int index)
{
    l.choice = index;
    if (l.choice == 0) { // FREQUENCY
        label = QString::number (l.amount);
        label += " cents";
        ui->label_LFO_amount->setText(label);
    } else if (l.choice == 1) { // RATIO
        label = QString::number (l.amount / 10);
        label += "%";
        ui->label_LFO_amount->setText(label);
    } else if (l.choice == 2) { // FILTER
        label = QString::number (l.amount);
        label += " Hz";
        ui->label_LFO_amount->setText(label);
        l.f_cutoff  = f.cutoff;
        initialize_filter();
    } else if (l.choice == 3) { // DISTORTION
        if (d.type == 0) { // overdrive
            label = QString:: number (l.amount / 100.0);
            ui->label_LFO_amount->setText(label);
        } else if (d.type == 1) { //bitcrusher
            label = QString:: number (l.amount / 100);
            label += " bits";
            ui->label_LFO_amount->setText(label);
        }
    }
}

void MainWindow::on_horizontalSlider_LFO_amount_valueChanged(int value)
{
    l.amount = value;
    if (l.choice == 0) { // FREQUENCY
        label = QString::number (l.amount);
        label += " cents";
        ui->label_LFO_amount->setText(label);
    } else if (l.choice == 1) { // RATIO
        label = QString::number (l.amount / 10);
        label += "%";
        ui->label_LFO_amount->setText(label);
    } else if (l.choice == 2) { // FILTER
        label = QString::number (l.amount);
        label += " Hz";
        ui->label_LFO_amount->setText(label);
    } else if (l.choice == 3) { // DISTORTION
        if (d.type == 0) { // overdrive
            label = QString:: number (l.amount / 100.0);
            ui->label_LFO_amount->setText(label);
        } else if (d.type == 1) { //bitcrusher
            label = QString:: number (l.amount / 100);
            label += " bits";
            ui->label_LFO_amount->setText(label);
        }
    }
}

// filter

void MainWindow::on_pushButton_filter_clicked()
{
    f.filter_enabled = !f.filter_enabled;
    if (f.filter_enabled) initialize_filter();
    if (f.filter_enabled) label = "On"; else label = "Off";
    ui->pushButton_filter->setText(label);
}

void MainWindow::on_comboBox_filter_currentIndexChanged(int index)
{
    f.filter_type = index;
    initialize_filter();
}

void MainWindow::on_horizontalSlider_cutoff_valueChanged(int value)
{
    f.cutoff = value;
    initialize_filter();
    label = QString::number(f.cutoff);
    label += " Hz";
    ui->label_cutoff->setText(label);
}

void MainWindow::on_dial_res_valueChanged(int value)
{
    f.resonance = value / 100.0;
    initialize_filter();
}

// distortion

void MainWindow::on_pushButton_dist_clicked()
{
    d.enabled = !d.enabled;
    if (d.enabled) label = "On"; else label = "Off";
    ui->pushButton_dist->setText(label);
}

void MainWindow::on_comboBox_dist_currentIndexChanged(int index)
{
    d.type = index;
    int value = ui->horizontalSlider_dist_amount->value();
    if (d.type == 0) { // overdrive
        d.o_amount = ((value * 15) / 224.0) + 1; // range is 0 to 5
        label = QString::number(d.o_amount);
        // label += " times";
        ui->label_dist_amount->setText(label);
    } else if (d.type == 1) { // bitcrushing
        d.b_amount = 16 - value/16; // most distortion is 16 - 14 - 2 bit
        label = QString::number(d.b_amount);
        label += " bits";
        ui->label_dist_amount->setText(label);
    }
    if (l.choice == 3){ // if LFO affects distortion, we need to update it also
        if (d.type == 0) { // overdrive
            label = QString:: number (l.amount / 100.0);
            ui->label_LFO_amount->setText(label);
        } else if (d.type == 1) { //bitcrusher
            label = QString:: number (l.amount / 100);
            label += " bits";
            ui->label_LFO_amount->setText(label);
        }
    }
}

void MainWindow::on_horizontalSlider_dist_amount_valueChanged(int value)
{
    // value goes from 0 - 224
    if (d.type == 0) { // overdrive
        d.o_amount = ((value * 15) / 224.0) + 1; // range is 0 to 5
        label = QString::number(d.o_amount);
        // label += " times";
        ui->label_dist_amount->setText(label);
    } else if (d.type == 1) { // bitcrushing
        d.b_amount = 16 - value/16; // most distortion is 16 - 14 - 2 bit
        label = QString::number(d.b_amount);
        label += " bits";
        ui->label_dist_amount->setText(label);
    }
}

// other controls

void MainWindow::on_verticalSlider_gain_valueChanged(int value)
{
    g.output_volume = value/100.0;
}

void MainWindow::on_comboBox_midi_currentIndexChanged(int index)
{
    g.midi_device = index;
    chooseMidiPort(g.midi_device);
}


void MainWindow::on_comboBox_LFO_wave_currentIndexChanged(int index)
{
    l.osc_type = index;
}

// pitch bend
void MainWindow::on_horizontalSlider_pbend_valueChanged(int value)
{
    g.pbend_range = value;
    label = QString::number(value);
    label += " cents";
    ui->label_pbend->setText(label);
}

void MainWindow::on_actionCrunchy_Bass_triggered()
{
    ui->comboBox_osc1->setCurrentIndex(1);           // saw
    ui->comboBox_osc2->setCurrentIndex(3);           // square
    ui->verticalSlider_oscRatio->setValue(50);       // 50% ratio
    if (!l.enabled)
        ui->pushButton_LFO->click();                 // turn on LFO
    ui->comboBox_LFO->setCurrentIndex(1);            // LFO affects ratio
    ui->horizontalSlider_LFO->setValue(20);          // LFO freq is 2 hz
    ui->horizontalSlider_LFO_amount->setValue(500);  // LFO range is 50%
    ui->comboBox_LFO_wave->setCurrentIndex(2);       // LFO triangle
    if (f.filter_enabled)
        ui->pushButton_filter->click();              // don't want filter
    if (!d.enabled)
        ui->pushButton_dist->click();                // turn on distortion
    ui->comboBox_dist->setCurrentIndex(1);           // bitcrushing
    ui->horizontalSlider_dist_amount->setValue(224); // maximum bitcrushing
    ui->verticalSlider_attack->setValue(1);          // super short attack
    ui->verticalSlider_decay->setValue(1);           // no decay
    ui->verticalSlider_sustain->setValue(99);        // sustain level max
    ui->verticalSlider_release->setValue(1);         // short release time
}

void MainWindow::on_actionLevel_Up_triggered()
{
    ui->comboBox_osc1->setCurrentIndex(2);           // triangle
    ui->comboBox_osc2->setCurrentIndex(3);           // square
    ui->verticalSlider_oscRatio->setValue(50);       // 50% ratio
    if (!l.enabled)
        ui->pushButton_LFO->click();                 // turn on LFO
    ui->comboBox_LFO->setCurrentIndex(0);            // LFO affects frequency
    ui->horizontalSlider_LFO->setValue(80);          // LFO freq is 8 hz
    ui->horizontalSlider_LFO_amount->setValue(500);  // LFO range is 50%
    ui->comboBox_LFO_wave->setCurrentIndex(3);       // LFO square
    if (f.filter_enabled)
        ui->pushButton_filter->click();              // don't want filter
    if (!d.enabled)
        ui->pushButton_dist->click();                // turn on distortion
    ui->comboBox_dist->setCurrentIndex(1);           // bitcrushing
    ui->horizontalSlider_dist_amount->setValue(224); // maximum bitcrushing
    ui->verticalSlider_attack->setValue(1);          // super short attack
    ui->verticalSlider_decay->setValue(1);           // no decay
    ui->verticalSlider_sustain->setValue(99);        // sustain level max
    ui->verticalSlider_release->setValue(500);       // one second release time
}

void MainWindow::on_actionDub_Bass_triggered()
{
    ui->comboBox_osc1->setCurrentIndex(1);           // saw
    ui->comboBox_osc2->setCurrentIndex(3);           // square
    ui->verticalSlider_oscRatio->setValue(50);       // 50% ratio
    if (!l.enabled)
        ui->pushButton_LFO->click();                 // turn on LFO
    ui->comboBox_LFO->setCurrentIndex(2);            // LFO affects filter
    ui->horizontalSlider_LFO->setValue(30);          // LFO freq is 2 hz
    ui->horizontalSlider_LFO_amount->setValue(500);  // LFO range is 500 hz
    ui->comboBox_LFO_wave->setCurrentIndex(1);       // LFO saw
    if (!f.filter_enabled)
        ui->pushButton_filter->click();              // want filter
    ui->comboBox_filter->setCurrentIndex(0);         // lowpass filter
    ui->horizontalSlider_cutoff->setValue(600);      // cutoff is 600 hz
    if (!d.enabled)
        ui->pushButton_dist->click();                // turn on distortion
    ui->comboBox_dist->setCurrentIndex(0);           // overdrive
    ui->horizontalSlider_dist_amount->setValue(200); // a fair amount of overdrive
    ui->verticalSlider_attack->setValue(10);          // short attack
    ui->verticalSlider_decay->setValue(10);           // short decay
    ui->verticalSlider_sustain->setValue(50);        // sustain level max
    ui->verticalSlider_release->setValue(1);         // short release
}

void MainWindow::on_actionSmooth_Lead_triggered()
{
    ui->comboBox_osc1->setCurrentIndex(2);           // triangle
    ui->comboBox_osc2->setCurrentIndex(0);           // sine
    ui->verticalSlider_oscRatio->setValue(25);
    if (!l.enabled)
        ui->pushButton_LFO->click();                 // turn on LFO
    ui->comboBox_LFO->setCurrentIndex(1);            // LFO affects ratio
    ui->horizontalSlider_LFO->setValue(40);          // LFO freq is 2 hz
    ui->horizontalSlider_LFO_amount->setValue(500);  // LFO range is 50 %
    ui->comboBox_LFO_wave->setCurrentIndex(1);       // LFO saw
    if (!f.filter_enabled)
        ui->pushButton_filter->click();              // want filter
    ui->comboBox_filter->setCurrentIndex(1);         // hipass filter
    ui->horizontalSlider_cutoff->setValue(775);      // cutoff is 775 hz
    if (d.enabled)
        ui->pushButton_dist->click();                // turn off distortion
    ui->verticalSlider_attack->setValue(50);
    ui->verticalSlider_decay->setValue(15);
    ui->verticalSlider_sustain->setValue(80);        // sustain level high
    ui->verticalSlider_release->setValue(200);         // long release
}

void MainWindow::on_actionBusy_Bee_triggered()
{
    ui->comboBox_osc1->setCurrentIndex(1);           // triangle
    ui->comboBox_osc2->setCurrentIndex(1);           // sine
    if (!l.enabled)
        ui->pushButton_LFO->click();                 // turn on LFO
    ui->comboBox_LFO->setCurrentIndex(0);            // LFO affects frequency
    ui->horizontalSlider_LFO->setValue(90);
    ui->horizontalSlider_LFO_amount->setValue(60);  // LFO range is 50 %
    ui->comboBox_LFO_wave->setCurrentIndex(2);       // LFO triangle
    if (!f.filter_enabled)
        ui->pushButton_filter->click();              // want filter
    ui->comboBox_filter->setCurrentIndex(0);         // lopass filter
    ui->horizontalSlider_cutoff->setValue(800);      // cutoff is 800 hz
    if (!d.enabled)
        ui->pushButton_dist->click();                // turn on distortion
    ui->comboBox_dist->setCurrentIndex(0);           // overdrive
    ui->horizontalSlider_dist_amount->setValue(12); // a fair amount of overdrive
    ui->verticalSlider_attack->setValue(1);
    ui->verticalSlider_decay->setValue(1);
    ui->verticalSlider_sustain->setValue(99);        // sustain level high
    ui->verticalSlider_release->setValue(1);         // long release
}

void MainWindow::on_actionMotor_triggered()
{
    ui->comboBox_osc1->setCurrentIndex(1);           // triangle
    ui->comboBox_osc2->setCurrentIndex(3);           // square
    if (!l.enabled)
        ui->pushButton_LFO->click();                 // turn on LFO
    ui->comboBox_LFO->setCurrentIndex(0);            // LFO affects frequency
    ui->horizontalSlider_LFO->setValue(7);
    ui->horizontalSlider_LFO_amount->setValue(99);  // LFO range is 99 cents
    ui->comboBox_LFO_wave->setCurrentIndex(1);       // LFO saw
    if (!f.filter_enabled)
        ui->pushButton_filter->click();              // want filter
    ui->comboBox_filter->setCurrentIndex(0);         // lopass filter
    ui->horizontalSlider_cutoff->setValue(999);      // cutoff is 800 hz
    if (!d.enabled)
        ui->pushButton_dist->click();                // turn on distortion
    ui->comboBox_dist->setCurrentIndex(0);           // overdrive
    ui->horizontalSlider_dist_amount->setValue(224); // maximum overdrive
    ui->verticalSlider_attack->setValue(1);
    ui->verticalSlider_decay->setValue(1);
    ui->verticalSlider_sustain->setValue(99);        // sustain level high
    ui->verticalSlider_release->setValue(1);
}
