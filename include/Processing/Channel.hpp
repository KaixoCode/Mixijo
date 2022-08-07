#pragma once
#include "pch.hpp"

namespace Mixijo {
	struct Compressor {
#define accuratelin2db(lin) (20.0 * std::log10(static_cast<double>(lin)))
#define accuratedb2lin(db) std::pow(10.0, 0.05 * (db))
		double sampleRate = 48000;
		constexpr static double DC_OFFSET = 1.0E-25;

		double compressThreshhold = -40;
		double compressRatio = 1.0 / 8.0;

		double compressEnvelope = DC_OFFSET;
		double attackInMillis = 1;
		double releaseInMillis = 100;
		double attackCoefficient = std::exp(-1.0 / ((attackInMillis / 1000.0) * sampleRate));
		double releaseCoefficient = std::exp(-1.0 / ((releaseInMillis / 1000.0) * sampleRate));

		double compressMult = 1;
		double biggest = 0;

		int zeroCounter = 0;

		float coeficient(float ms) { return std::exp(-1.0 / ((ms / 1000.0) * sampleRate)); }

		void attack(float ms) {
			if (ms == attackInMillis) return;
			attackInMillis = ms;
			attackCoefficient = coeficient(attackInMillis);
		}

		void release(float ms) {
			if (ms == releaseInMillis) return;
			releaseInMillis = ms;
			releaseCoefficient = coeficient(releaseInMillis);
		}

		double process(double sin, int c) {
			if (sin == 0 && zeroCounter <= 100) zeroCounter++;
			else if (sin != 0) zeroCounter = 0;

			if (zeroCounter > 100) return 0;

			double _abs = std::abs(sin);
			if (biggest < _abs) biggest = _abs;

			if (c != 0) return sin * compressMult;
			else {
				double _sample = biggest;
				biggest = 0;

				_sample += DC_OFFSET; // add DC offset to avoid log( 0 )
				_sample = accuratelin2db(_sample); // convert linear -> dB
				_sample = std::max(_sample - compressThreshhold, 0.0);

				_sample += DC_OFFSET; // add DC offset to avoid denormal	
				compressEnvelope = _sample + (
					_sample > compressEnvelope
						? attackCoefficient 
						: releaseCoefficient
					) * (compressEnvelope - _sample);
				_sample = compressEnvelope - DC_OFFSET;

				compressMult = accuratedb2lin(_sample * (compressRatio - 1.0));
				
				return sin * compressMult;
			}
		}

	};

	struct Limiter {
		Compressor compressor{
			.compressThreshhold = -3, // Limit at 0dB
			.compressRatio = 0, // Hard limiter
			.attackInMillis = 1,
			.releaseInMillis = 50,
		};

		std::vector<std::vector<double>> delayedBuffer;
		std::size_t delayedAccessor = 0;

		double process(double val, int channel) {
			auto& _buffer = delayedBuffer[channel];
			std::size_t _size = _buffer.size();

			// Access delayed sample
			double _sample = _buffer[delayedAccessor];
			// Write new sample into the future
			_buffer[(delayedAccessor + _size - 1) % _size] = val;

			if (channel == 0) delayedAccessor = (delayedAccessor + 1) % _size;
			return std::clamp(compressor.process(_sample, channel), -1., 1.);
		}
	};

    struct Channel {
        std::vector<int> endpoints{};
        std::vector<double> values{};
        std::vector<double> peaks{};

		Limiter limiter;

        enum MidiLink { Gain };

        std::map<int, MidiLink> midiLinks;
        double gain = 1;
		bool enableLimiter = false;

        void getSettings(std::ofstream& file);
        void setSetting(std::string_view name, double val);
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