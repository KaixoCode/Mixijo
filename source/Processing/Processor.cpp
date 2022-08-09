#include "Processing/Processor.hpp"
#include "Controller.hpp"

namespace Mixijo {

    InputChannel& Processor::Inputs::add() {
        auto& _channel = _data.emplace_back();
        _channel.output_levels.resize(self.outputs.size());
        return _channel;
    }

    void Processor::Inputs::remove(int index) {
        _data.erase(_data.begin() + index);
    }

    OutputChannel& Processor::Outputs::add() {
        auto& _channel = _data.emplace_back();
        for (auto& _input : self.inputs)
            _input.output_levels.resize(_data.size());
        return _channel;
    }

    void Processor::Outputs::remove(int index) {
        _data.erase(_data.begin() + index);
        for (auto& _input : self.inputs)
            _input.output_levels.erase(_input.output_levels.begin() + index);
    }

    bool Processor::init() {
        if (initAudio() != Audijo::NoError) return false;
        if (initMidi() != Midijo::NoError) return false;
        return true;
    }

    Audijo::Error Processor::initAudio() {
        Callback(callback);
        UserData(*this);
        auto& devices = Devices(true);
        for (auto& _device : devices) {
            if (_device.name == Controller::audioDevice) {
                Audijo::Error _res = Open({ 
                    .input = _device.id, 
                    .output = _device.id,
                    .bufferSize = Controller::bufferSize,
                    .sampleRate = Controller::sampleRate,
                });
                if (_res == Audijo::NoError) _res = Start();
                return _res;
            }
        }
    }

    Midijo::Error Processor::initMidi() {
        midiin.Callback([&](const Midijo::Event& e) {
            if (midiout.Information().state == Midijo::Opened)
                midiout.Message(e);
        });
        midiin.Callback([&](const Midijo::CC& e) {
            for (auto& _in : inputs) _in.handleMidi(e.Number(), e.Value());
            for (auto& _out : outputs) _out.handleMidi(e.Number(), e.Value());
        });
        auto& _indevices = midiin.Devices(true);
        for (auto& _device : _indevices)
            if (_device.name == Controller::midiinDevice)
                midiin.Open({ .device = _device.id, .async = false });
        auto& _outdevices = midiout.Devices(true);
        for (auto& _device : _outdevices)
            if (_device.name == Controller::midioutDevice)
                midiout.Open({ .device = _device.id, .async = false });
        return Midijo::NoError;
    }

    void Processor::deinit() {
        midiin.Close();
        midiout.Close();
        Close();
    }

    void Processor::callback(Buffer<double>& in, Buffer<double>& out, CallbackInfo info, Processor& self) {
        std::scoped_lock _{ self.lock };
        for (std::size_t i = 0; i < in.Frames(); ++i) {
            auto _in_frame = in[i], _out_frame = out[i];
            for (auto& _endpoint : _out_frame) _endpoint = 0;
            for (auto& _output : self.outputs) _output.clear();
            for (auto& _input : self.inputs) {
                _input.generate(_in_frame);
                for (std::size_t j = 0; double _level : _input.output_levels)
                    self.outputs[j++].receive(_input.values, _level);
            }
            for (auto& _output : self.outputs) _output.generate(_out_frame);
            for (auto& _endpoint : _out_frame) _endpoint = std::clamp(_endpoint, -1., 1.);
        }
    }

    int Processor::find_endpoint(std::string_view name, bool in) {
        if (Information().state == Audijo::StreamState::Closed) return -1;
        for (auto& _channel : endpoints())
            if (_channel.input == in && _channel.name == name) return _channel.id;
        return -1;
    }
}