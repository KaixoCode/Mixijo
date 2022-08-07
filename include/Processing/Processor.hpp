#pragma once
#include "pch.hpp"
#include "Processing/Channel.hpp"

namespace Mixijo {
    struct Processor : Stream<Audijo::Api::Asio> {
        template<class Type> struct Storage {
            auto begin(this auto& self) { return self._data.begin(); }
            auto end(this auto& self) { return self._data.end(); }
            std::size_t size() const { return _data.size(); }
            void clear() { _data.clear(); }
            decltype(auto) operator[](this auto& self, std::size_t i) { return self._data[i]; }
        protected:
            std::vector<Type> _data;
            Processor& self;
            Storage(Processor& self) : self(self) {}
            friend class Processor;
        };

        struct Inputs : Storage<InputChannel> {
            InputChannel& add();
            void remove(int index);
        };

        struct Outputs : Storage<OutputChannel> {
            OutputChannel& add();
            void remove(int index);
        };

        MidiIn<Midijo::Windows> midiin;
        MidiOut<Midijo::Windows> midiout;

        std::vector<ChannelInfo>& endpoints() { return Device(Information().input).Channels(); }

        bool init();
        Audijo::Error initAudio();
        Midijo::Error initMidi();
        void deinit();

        static void callback(Buffer<double>& in, Buffer<double>& out, CallbackInfo info, Processor& self);

        /**
         * Find endpoint given its name.
         * @param name name of endpoint
         * @return id of found endpoint
         */
        int find_endpoint(std::string_view name, bool in);

        /**
         * Provides threadsafe access to the input and output channels
         * by calling the provided lambda after constructing a scoped lock.
         * @tparam lambda callable that takes the inputs and outputs as args
         */
        void access(std::invocable<Inputs&, Outputs&> auto lambda) {
            std::scoped_lock _{ lock };
            lambda(inputs, outputs);
        }

        Inputs inputs{ *this };
        Outputs outputs{ *this };
        mutable std::mutex lock;
    };
}