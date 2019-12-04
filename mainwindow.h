#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_comboBox_osc1_currentIndexChanged(int index);

    void on_comboBox_osc2_currentIndexChanged(int index);

    void on_verticalSlider_oscRatio_valueChanged(int value);

    void on_verticalSlider_attack_valueChanged(int value);

    void on_verticalSlider_decay_valueChanged(int value);

    void on_verticalSlider_sustain_valueChanged(int value);

    void on_verticalSlider_release_valueChanged(int value);

    void on_pushButton_LFO_clicked();

    void on_horizontalSlider_LFO_valueChanged(int value);

    void on_pushButton_filter_clicked();

    void on_comboBox_filter_currentIndexChanged(int index);

    void on_horizontalSlider_cutoff_valueChanged(int value);

    void on_dial_res_valueChanged(int value);

    void on_verticalSlider_gain_valueChanged(int value);

    void on_comboBox_midi_currentIndexChanged(int index);

    void on_comboBox_LFO_currentIndexChanged(int index);

    void on_horizontalSlider_LFO_amount_valueChanged(int value);

    void on_comboBox_dist_currentIndexChanged(int index);

    void on_horizontalSlider_dist_amount_valueChanged(int value);

    void on_pushButton_dist_clicked();

    void on_comboBox_LFO_wave_currentIndexChanged(int index);

    void on_horizontalSlider_pbend_valueChanged(int value);

    void on_actionCrunchy_Bass_triggered();

    void on_actionLevel_Up_triggered();

    void on_actionDub_Bass_triggered();

    void on_actionSmooth_Lead_triggered();

    void on_actionBusy_Bee_triggered();

    void on_actionMotor_triggered();

private:
    Ui::MainWindow *ui;

    void init_controls(std::vector<std::string>);
};

#endif // MAINWINDOW_H
