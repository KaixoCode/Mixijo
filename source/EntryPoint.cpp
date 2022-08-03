#include "pch.hpp"

int main() {

    Guijo::Gui _gui;
    Midijo::MidiIn _midi;
    Audijo::Stream<Audijo::Api::Asio> _stream;

    _gui.emplace<Guijo::Window>(Guijo::Window::Construct{
        .name = "Window",
        .dimensions { -1, -1, 500, 500 },
    });

    while (_gui.loop());
}