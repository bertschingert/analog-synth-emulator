#include "mainwindow.h"
#include "audio.h"
#include "midi.h"
#include <thread>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    std::thread audio_thread(initialize_audio);
    std::thread midi_thread(midi_watcher);

    w.show();

    int ret = a.exec();
    midi_thread.join();
    audio_thread.join();
    return ret;
}
