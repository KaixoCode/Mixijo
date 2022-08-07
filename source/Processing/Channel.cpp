#include "Processing/Channel.hpp"

namespace Mixijo {
    
    void Channel::addMidiLink(std::string_view name, int id) {
        if (name == "gain") midiLinks[id] = Gain;
    }

    void Channel::handleMidi(int id, int value) {
        if (!midiLinks.contains(id)) return;
        else switch (midiLinks.at(id)) {
            case Gain: gain = std::pow(1.412536 * value / 127., 4); break;
        }
    }

    void Channel::add(int endpoint) {
        endpoints.push_back(endpoint);
        values.resize(endpoints.size());
        peaks.resize(endpoints.size());
    }

    void Channel::remove(int endpoint) {
        endpoints.erase(std::remove(endpoints.begin(), endpoints.end(), endpoint), endpoints.end());
        values.resize(endpoints.size());
        peaks.resize(endpoints.size());
    }

    void Channel::process() {
        for (std::size_t i = 0; i < values.size(); ++i) {
            values[i] *= gain;
            const double _abs = std::abs(values[i]);
            peaks[i] = std::max(_abs, peaks[i]);
        }
    }

    void InputChannel::generate(Buffer<double>::Frame& frame) {
        for (std::size_t i = 0; int _endpoint : endpoints) 
            values[i] = frame[_endpoint], ++i;
        process();
    }

    void OutputChannel::receive(const std::vector<double>& in, double level) {
        if (values.size() == 0) return;
        for (std::size_t i = 0; i < std::max(in.size(), values.size()); ++i)
            values[i % values.size()] += in[i % in.size()] * level;
    }

    void OutputChannel::clear() {
        for (auto& _v : values) _v = 0;
    }

    void OutputChannel::generate(Buffer<double>::Frame& frame) {
        process();
        for (std::size_t i = 0; int _endpoint : endpoints)
            frame[_endpoint] += values[i], ++i;
    }
}