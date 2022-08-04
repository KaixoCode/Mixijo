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

    void generate(Buffer<double>::Frame& frame) {
        // Get samples from the endpoints in the frame
        for (std::size_t i = 0; int _endpoint : endpoints) {
            assert(_endpoint < frame.Channels());
            values[i] = frame[_endpoint], ++i;
        }
        process(); // Process the values
    }
};

struct OutputChannel : Channel {
    void receive(const std::vector<double>& in) {
        // TODO: send data to 'values'
    }

    void generate(Buffer<double>::Frame& frame) {
        process(); // Process its values
        // Output the values to the endpoints in the frame
        for (std::size_t i = 0; int _endpoint : endpoints) {
            assert(_endpoint < frame.Channels());
            frame[_endpoint] += values[i], values[i] = 0, ++i;
        }
    }
};

struct Audio : Stream<Audijo::Api::Asio> {
    Audio() { init(); }

    Audijo::Error init() {
        Callback(callback);
        UserData(*this);
        for (auto& _device : Devices()) {
            if (_device.name == "Synchronous Audio Router") {
                Audijo::Error _res = Open({ .input = _device.id, .output = _device.id });
                if (_res == Audijo::NoError) _res = Start();
                return _res;
            }
        }
    }

    static void callback(Buffer<double>& in, Buffer<double>& out, CallbackInfo info, Audio& self) {
        std::scoped_lock _{ self.lock };
        for (std::size_t i = 0; i < in.Frames(); ++i) {
            auto _in_frame = in[i], _out_frame = out[i];
            // Mute the output
            for (auto& _endpoint : _out_frame)
                _endpoint = 0;
            // Take the input signal and send it to the input channels.
            for (auto& _input : self.inputs) {
                _input.generate(_in_frame);
                // Forward the channels to their connections.
                for (int _output_id : _input.connections) {
                    assert(_output_id < self.outputs.size());
                    self.outputs[_output_id].receive(_input.values);
                }
            }
            // Finally, output the output channels.
            for (auto& _output : self.outputs)
                _output.generate(_out_frame);
        }
    }

    /**
     * Wrapper for an std::vector that has limited access to the 
     * underlying data structure. So the access can be managed 
     * properly to make sure the state of all channels remains valid.
     */
    template<class Type> struct Storage {
        auto begin(this auto& self) { return self._data.begin(); }
        auto end(this auto& self) { return self._data.end(); }
        std::size_t size() const { return _data.size(); }
        decltype(auto) operator[](this auto& self, std::size_t i) { return self._data[i]; }
        void add() { _data.emplace_back(); }
    protected:
        std::vector<Type> _data;
        Audio& self;
        Storage(Audio& self) : self(self) {}
        friend class Audio;
    };

    struct Inputs : Storage<InputChannel> {
        void remove(int index) { _data.erase(_data.begin() + index); }
    };

    struct Outputs : Storage<OutputChannel> {
        void remove(int index) {
            // Remove at index
            _data.erase(_data.begin() + index);
            // Update input connections accordingly
            for (auto& _input : self.inputs) {
                auto _it = _input.connections.begin();
                while (_it != _input.connections.end()) {
                    // If connection was index, remove as connection
                    if (*_it == index) _it = _input.connections.erase(_it);
                    // If connection > index, remove 1 from connection because 
                    // output was removed, so all indices after it shift over by 1.
                    else if (*_it > index) (*_it)--, ++_it;
                    else ++_it;
                }
            }
        }
    };

    /**
     * Provides threadsafe access to the input and output channels
     * by calling the provided lambda after constructing a scoped lock.
     * @tparam lambda callable that takes the inputs and outputs as args
     */
    void access(std::invocable<Inputs&, Outputs&> auto lambda) {
        std::scoped_lock _{ lock };
        lambda(inputs, outputs);
    }

private:
    mutable std::mutex lock;
    Inputs inputs{ *this };
    Outputs outputs{ *this };
};

int main() {

    Gui _gui;
    MidiIn _midi;
    Audio _audio;

    _gui.emplace<Window>(Window::Construct{
        .name = "Window",
        .dimensions { -1, -1, 500, 500 },
    });

    while (_gui.loop()) {

    }
}