#pragma once
#include "pch.hpp"

namespace Mixijo {
    struct Channel {
        std::vector<int> endpoints{};
        std::vector<double> values{};
        std::vector<double> peaks{};

        enum MidiLink { Gain };

        std::map<int, MidiLink> midiLinks;
        double gain = 1;

        void addMidiLink(std::string_view name, int id);
        void handleMidi(int id, int value);
        void add(int endpoint);
        void remove(int endpoint);
        void process();
    };

    struct InputChannel : Channel {
        std::vector<double> output_levels{};

        void generate(Buffer<double>::Frame& frame);
    };

    struct OutputChannel : Channel {
        void receive(const std::vector<double>& in, double level);
        void clear();
        void generate(Buffer<double>::Frame& frame);
    };
}