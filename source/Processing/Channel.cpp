#include "Processing/Channel.hpp"
#include "Controller.hpp"

namespace Mixijo {
    

    void Channel::getSettings(std::ofstream& file) {
        file << "gain=" << gain << ",limiter=" << (enableLimiter ? 1 : 0);
    }

    void Channel::setSetting(std::string_view name, double val) {
        if (name == "gain") gain = val;
        if (name == "limiter") enableLimiter = val;
    }

    void Channel::addMidiLink(std::string_view name, int id) {
        if (name == "gain") midiLinks[id] = Gain;
    }

    void Channel::handleMidi(int id, int value) {
        if (!midiLinks.contains(id)) return;
        else switch (midiLinks.at(id)) {
            case Gain: gain = std::pow(Controller::maxLin * value / 127., 4); break;
        }
    }

    void Channel::add(int endpoint) {
        endpoints.push_back(endpoint);
        values.resize(endpoints.size());
        peaks.resize(endpoints.size());
        limiter.delayedBuffer.emplace_back();
        for (auto& _buffer : limiter.delayedBuffer)
            _buffer.resize(144);
    }

    void Channel::remove(int endpoint) {
        endpoints.erase(std::remove(endpoints.begin(), endpoints.end(), endpoint), endpoints.end());
        values.resize(endpoints.size());
        peaks.resize(endpoints.size());
    }

    void Channel::process() {
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (enableLimiter) values[i] = limiter.process(values[i], i) * gain;
            else values[i] *= gain;
            const double _abs = std::abs(values[i]);
            peaks[i] = std::max(_abs, peaks[i]);
        }
    }

    void InputChannel::generate(Buffer<double>::Frame& frame) {
        idle = true;
        for (std::size_t i = 0; int _endpoint : endpoints) {
            values[i] = frame[_endpoint] * gain;
            peaks[i] = std::max(std::abs(values[i]), peaks[i]);
            idle &= values[i] == 0;
            ++i;
        }
    }

    void OutputChannel::receive(const std::vector<double>& in, double level) {
        const auto _valSize = values.size();
        const auto _inSize = in.size();
        if (_valSize == 0 || _inSize == 0) return;
        if (_valSize == _inSize) {
            for (std::size_t i = 0; i < _valSize; ++i)
                values[i] += in[i] * level;
        } else if (_valSize > _inSize) {
            for (std::size_t i = 0; i < _valSize; ++i)
                values[i] += in[i % _inSize] * level;
        } else {
            for (std::size_t i = 0; i < _inSize; ++i)
                values[i % _valSize] += in[i] * level;
        }
    }

    void OutputChannel::clear() {
        for (auto& _v : values) _v = 0;
    }

    void OutputChannel::generate(Buffer<double>::Frame& frame) {
        for (std::size_t i = 0; int _endpoint : endpoints) {
            frame[_endpoint] += values[i] * gain;
            peaks[i] = std::max(std::abs(values[i]), peaks[i]);
            values[i] = 0;
            ++i;
        }
    }
}