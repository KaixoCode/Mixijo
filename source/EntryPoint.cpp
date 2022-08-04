#include "pch.hpp"

/*

 Features:
  - Group channels
  - Route channels
  - Controls:
      - Volume
      - Limiter
  - Midi control of volume
  - Level meter
  - Forward Midi




  inputs [0, 1, 2, 3, 4, 5, 6]
    
    Input Channel [0, 4] -> Output Channel [0, 1]

  outputs [0, 1, 2, 3]



*/

struct Channel {
    std::vector<int> endpoints{};
    std::vector<double> values{};

    void add(int endpoint) {
        endpoints.push_back(endpoint);
        values.reserve(endpoints.size());
    }

    void remove(int endpoint) {
        endpoints.erase(std::remove(endpoints.begin(), endpoints.end(), endpoint), endpoints.end());
        values.reserve(endpoints.size());
    }

    void process() {
        // Process 'values'
    }
};

struct InputChannel : Channel {
    std::vector<int> connections{};

    void generate(Audijo::Buffer<double>::Frame& frame) {
        for (std::size_t i = 0; int _endpoint : endpoints) {
            assert(_endpoint < frame.Channels());
            values[i] = frame[_endpoint], ++i;
        }
        process();
    }
};

struct OutputChannel : Channel {
    void receive(const std::vector<double>& in) {
        // TODO: send data to 'values'
    }

    void generate(Audijo::Buffer<double>::Frame& frame) {
        process();
        for (std::size_t i = 0; int _endpoint : endpoints) {
            assert(_endpoint < frame.Channels());
            frame[_endpoint] += values[i], values[i] = 0, ++i;
        }
    }
};

struct Audio : Audijo::Stream<Audijo::Api::Asio> {
    Audio() { init(); }

    void init() {
        Callback(callback);
        UserData(*this);
        for (auto& _device : Devices()) {
            if (_device.name == "Synchronous Audio Router") {
                Audijo::Error _res = Open({ .input = _device.id, .output = _device.id });
                if (_res == Audijo::NoError) Start();
                break;
            }
        }
    }

    static void callback(Audijo::Buffer<double>& in, Audijo::Buffer<double>& out, Audijo::CallbackInfo info, Audio& self) {
        std::lock_guard _{ self.lock };
        for (std::size_t i = 0; i < in.Frames(); ++i) {
            auto _in_frame = in[i];
            auto _out_frame = out[i];
            for (auto& _input : self.inputs) {
                _input.generate(_in_frame);
                for (int _output_id : _input.connections) {
                    assert(_output_id < self.outputs.size());
                    self.outputs[_output_id].receive(_input.values);
                }
            }

            for (auto& _output : self.outputs) 
                _output.generate(_out_frame);
        }
    }

    void access(std::invocable<std::vector<InputChannel>&, std::vector<OutputChannel>&> auto lambda) {
        std::lock_guard _{ lock };
        lambda(inputs, outputs);
    }

private:
    mutable std::mutex lock;
    std::vector<InputChannel> inputs;
    std::vector<OutputChannel> outputs;
};

int main() {

    Guijo::Gui _gui;
    Midijo::MidiIn _midi;
    Audio _audio;

    _audio.access([](auto&, std::vector<OutputChannel>& a) {
    });

    _gui.emplace<Guijo::Window>(Guijo::Window::Construct{
        .name = "Window",
        .dimensions { -1, -1, 500, 500 },
    });

    while (_gui.loop()) {

    }
}