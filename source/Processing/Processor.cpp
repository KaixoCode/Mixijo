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

    Processor::Processor() {
        midiin.Callback([&](const Midijo::Event& e) {
            if (midiout.Information().state == Midijo::Opened)
                midiout.Message(e);
        });
        midiin.Callback([&](const Midijo::CC& e) {
            for (auto& _in : inputs) _in.handleMidi(e.Number(), e.Value());
            for (auto& _out : outputs) _out.handleMidi(e.Number(), e.Value());
        });
    }

    bool Processor::init() {
        deinit();
        bool _success = true;
        if (initAudio() != Audijo::NoError) _success = false;
        if (initMidi() != Midijo::NoError) _success = false;
        return true;
    }

    Audijo::Error Processor::initAudio() {
        Callback(callback);
        UserData(*this);
        auto& devices = Devices(true);
        for (auto& _device : devices) {
            if (_device.name == Controller::audioDevice) {
                Controller::logline("Opening device (", _device.name, ")");
                Controller::logline("  id:         ", _device.id);
                Controller::logline("  inputs:     ", _device.inputChannels);
                Controller::logline("  outputs:    ", _device.outputChannels);
                Controller::logline("  samplerate: ", Controller::sampleRate);
                Controller::logline("  buffersize: ", Controller::bufferSize);
                Controller::logline("  channels:   ");

                for (auto& channel : _device.Channels()) {
                    Controller::logline("    ", channel.name, " [", channel.id, "] - ", channel.input ? "input" : "output");
                }

                Audijo::Error _res = Open({ 
                    .input = _device.id, 
                    .output = _device.id,
                    .bufferSize = Controller::bufferSize,
                    .sampleRate = Controller::sampleRate,
                });
                if (_res == Audijo::NoError) _res = Start();
                if (_res != Audijo::NoError) {
                    switch (_res) {
                    case Audijo::InvalidBufferSize: Controller::errline("Invalid buffer size! (", Controller::bufferSize, ")"); break;
                    case Audijo::InvalidSampleRate: Controller::errline("Invalid samplerate! (", Controller::sampleRate, ")"); break;
                    case Audijo::NotPresent: Controller::errline("Device not present! (", Controller::audioDevice, ")"); break;
                    case Audijo::Fail: Controller::errline("Failed to open audio device (", Controller::audioDevice, ")"); break;
                    }
                }
                return _res;
            }
        }
        Controller::errline("No audio device found with the name (", Controller::audioDevice, ")");
        Controller::logline("available audio devices:");
        for (auto& device : Devices()) {
            Controller::logline("  " + device.name);
        }
    }

    Midijo::Error Processor::initMidi() {
        auto& _indevices = midiin.Devices(true);
        bool _found = false;
        for (auto& _device : _indevices) {
            if (_device.name == Controller::midiinDevice) {
                auto _res = midiin.Open({ .device = _device.id, .async = false });
                _found = true;
                if (_res != Midijo::NoError) {
                    switch (_res) {
                    case Midijo::InUse: Controller::errline("Midi-in device is already in use (", Controller::midiinDevice, ")"); break;
                    case Midijo::Fail: Controller::errline("Failed to open midi-in device (", Controller::midiinDevice, ")"); break;
                    }
                }
            }
        }

        if (!_found) {
            if (Controller::midiinDevice != "") 
                Controller::errline("No midi-in device found with the name (", Controller::midiinDevice, ")");
            Controller::logline("available midi-in devices:");
            for (auto& device : midiin.Devices()) {
                Controller::logline("  " + device.name);
            }
        }
        _found = false;
        auto& _outdevices = midiout.Devices(true);
        for (auto& _device : _outdevices) {
            if (_device.name == Controller::midioutDevice) {
                auto _res = midiout.Open({ .device = _device.id, .async = false });
                _found = true;
                if (_res != Midijo::NoError) {
                    switch (_res) {
                    case Midijo::InUse: Controller::errline("Midi-out device is already in use (", Controller::midioutDevice, ")"); break;
                    case Midijo::Fail: Controller::errline("Failed to open midi-out device (", Controller::midioutDevice, ")"); break;
                    }
                }
            }
        }

        if (!_found) {
            if (Controller::midioutDevice != "")
                Controller::errline("No midi-out device found with the name (", Controller::midioutDevice, ")");
            Controller::logline("available midi-out devices:");
            for (auto& device : midiout.Devices()) {
                Controller::logline("  " + device.name);
            }
        }
        return Midijo::NoError;
    }

    void Processor::deinit() {
        midiin.Close();
        midiout.Close();
        Close();
    }

    void Processor::callback(Buffer<double>& in, Buffer<double>& out, CallbackInfo info, Processor& self) {
        std::scoped_lock _{ self.lock };
        auto _frames = out.Frames();
        for (std::size_t i = 0; i < out.Channels(); ++i)
            std::memset(out.data()[i], 0, _frames * sizeof(double));
        for (std::size_t i = 0; i < _frames; ++i) {
            auto _in_frame = in[i], _out_frame = out[i];
            for (auto& _input : self.inputs) {
                _input.generate(_in_frame);
                if (_input.idle) continue;
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