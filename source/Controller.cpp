#include "Controller.hpp"
#include "Gui/Mixer.hpp"
#include "Gui/Channel.hpp"
#include "Utils.hpp"

namespace Mixijo {

    std::string Controller::audioDevice;
    std::string Controller::midiinDevice;
    std::string Controller::midioutDevice;
    int Controller::selectedChannel = -1;
    bool Controller::selectedInput = false;
    Processor Controller::processor{};
    Pointer<Window> Controller::window{};
    Controller::Theme Controller::theme;

    void Controller::loadSettings() {
        loadDevices();
        loadChannels();
        loadTheme();
        loadRouting();
    }

    void Controller::refreshDeviceNames() {
        std::ifstream _file{ "./devices.txt" };
        for (std::string _str; std::getline(_file, _str);) {
            std::string_view _view = _str;
            if (!_view.contains("=") || _view.starts_with("#")) continue;
            auto _parts = split(_view, '=');
            if (_parts.size() != 2) continue;
            auto _device = trim(_parts[0]);
            auto _name = trim(_parts[1]);
            if (_device == "audio") audioDevice = _name;
            if (_device == "midiin") midiinDevice = _name;
            if (_device == "midiout") midioutDevice = _name;
        }
    }

    void Controller::loadDevices() {
        processor.deinit();
        refreshDeviceNames();
        processor.init();
    }

    void Controller::loadTheme() {
        std::ifstream _file{ "./theme.txt" };
        theme.reset();

        for (std::string _str; std::getline(_file, _str);) {
            std::string_view _view = _str;
            if (!_view.contains("=") || _view.starts_with("#")) continue;
            auto _parts = split(_view, '=');
            if (_parts.size() != 2) continue; // Each theme component has 2 parts
            auto _selector = _parts[0];       // part 1: selector
            auto _value = _parts[1];          // part 2: value

            auto _assign = [](auto& part, std::string_view mod, auto value) {
                if (mod.contains("[")) {
                    mod = trim(mod, " \t\n\r\f\v[]");
                    if (mod == "Hovering") part.statelinked.push_back({ Hovering, value });
                    else if (mod == "Selected") part.statelinked.push_back({ Selected, value });
                    else if (mod == "Pressed") part.statelinked.push_back({ Pressed, value });
                    else if (mod == "Focused") part.statelinked.push_back({ Focused, value });
                    else if (mod == "Disabled") part.statelinked.push_back({ Disabled, value });
                } else if (mod.contains("transition")) {
                    if constexpr (!std::same_as<Color, decltype(value)>)
                        part.transition = value;
                } else {
                    part.value.emplace(value);
                }
            };

            auto _parseAs = [&]<class Ty>(std::type_identity<Ty>) {
                if constexpr (std::same_as<Ty, Color>) {
                    _value = trim(_value, " \t\n\r\f\v{}");
                    std::vector<std::string_view> _colorVec = split(_value, ',');
                    if (_colorVec.size() == 4) {
                        float _r = parse<float>(trim(_colorVec[0]));
                        float _g = parse<float>(trim(_colorVec[1]));
                        float _b = parse<float>(trim(_colorVec[2]));
                        float _a = parse<float>(trim(_colorVec[3]));
                        return Color{ _r, _g, _b, _a };
                    } else if (_colorVec.size() == 3) {
                        float _r = parse<float>(trim(_colorVec[0]));
                        float _g = parse<float>(trim(_colorVec[1]));
                        float _b = parse<float>(trim(_colorVec[2]));
                        return Color{ _r, _g, _b, 255.f };
                    } else if (_colorVec.size() == 2) {
                        float _g = parse<float>(trim(_colorVec[0]));
                        float _a = parse<float>(trim(_colorVec[1]));
                        return Color{ _g, _g, _g, _a };
                    } else if (_colorVec.size() == 1) {
                        float _g = parse<float>(trim(_colorVec[0]));
                        return Color{ _g, _g, _g, 255. };
                    }

                    return Color{};
                } else if constexpr (std::is_arithmetic_v<Ty>) {
                    return parse<Ty>(trim(_value));
                }
            };

            auto _tryValue = [&](std::string_view str, auto& part) {
                if (_selector.starts_with(str)) {
                    std::string_view _mod = _selector.substr(str.size());
                    using type = typename std::decay_t<decltype(part)>::type;
                    if (_mod.contains("transition")) {
                        _assign(part, _mod, _parseAs(std::type_identity<float>{}));
                    } else _assign(part, _mod, _parseAs(std::type_identity<type>{}));
                    return true;
                }
                return false;
            };

            if (_tryValue("divider", theme.divider)) continue;
            if (_tryValue("background", theme.background)) continue;
            if (_tryValue("routebutton.background", theme.routebutton.background)) continue;
            if (_tryValue("routebutton.borderWidth", theme.routebutton.borderWidth)) continue;
            if (_tryValue("routebutton.border", theme.routebutton.border)) continue;
            if (_tryValue("channel.background", theme.channel.background)) continue;
            if (_tryValue("channel.borderWidth", theme.channel.borderWidth)) continue;
            if (_tryValue("channel.meterBackground", theme.channel.meterBackground)) continue;
            if (_tryValue("channel.meterLine1", theme.channel.meterLine1)) continue;
            if (_tryValue("channel.meterLine2", theme.channel.meterLine2)) continue;
            if (_tryValue("channel.meterText", theme.channel.meterText)) continue;
            if (_tryValue("channel.meter", theme.channel.meter)) continue;
            if (_tryValue("channel.border", theme.channel.border)) continue;
            if (_tryValue("channel.title", theme.channel.title)) continue;
            if (_tryValue("channel.value", theme.channel.value)) continue;
        }

        auto _mixer = window->objects()[0].as<Gui::Mixer>();
        _mixer->updateTheme(); // Update theme in gui
    }

    void Controller::loadChannels() {
        // Get gui mixer object
        auto _mixer = window->objects()[0].as<Gui::Mixer>();
        _mixer->objects().clear(); 
        processor.access([](Processor::Inputs& in, Processor::Outputs& out) {
            in.clear();
            out.clear();
        });

        // Load channel settings
        std::ifstream _file{ "./channels.txt" };
        for (std::string _str; std::getline(_file, _str);) {
            std::string_view _view = _str;
            // # denotes comment line, ':' is part delimiter
            if (!_view.contains(":") || _view.starts_with("#")) continue;
            auto _parts = split(_view, ':');
            if (_parts.size() != 4) continue;                   // Each channel has 4 parts
            auto _type = trim(_parts[0]);                       // part 1: channel type: input/output
            auto _endpoints = trim(_parts[1], " \t\n\r\f\v[]"); // part 2: array with names all connected endpoints
            auto _name = trim(_parts[2]);                       // part 3: displayed name of channel
            auto _midi = trim(_parts[3], " \t\n\r\f\v[]");      // part 4: array of named midi links
            auto _input = _type == "input";

            // Access the processor channels in a threadsafe manner
            processor.access([&](Processor::Inputs& in, Processor::Outputs& out) {
                Channel& _channel = _input
                    ? static_cast<Channel&>(in.add())
                    : static_cast<Channel&>(out.add());

                std::vector<std::string_view> _endpointsVec = split(_endpoints, ',');
                for (std::string_view _e : _endpointsVec) {
                    _e = trim(_e);
                    int _id = processor.find_endpoint(_e, _input);
                    if (_id != -1) _channel.add(_id);
                }

                std::vector<std::string_view> _midiVec = split(_midi, ',');
                for (std::string_view _midiAssignment : _midiVec) {
                    _midiAssignment = trim(_midiAssignment);
                    auto _parts = split(_midiAssignment, '=');
                    if (_parts.size() != 2) continue; // 'name=CC id'
                    auto _param = _parts[0];                // part 1: parameter name
                    int _link = parse<int>(_parts[1]); // part 2: CC id, vvv parse int from string vvv
                    _channel.addMidiLink(_param, _link);
                }

                int size = _input ? in.size() : out.size();
                _mixer->emplace<Gui::Channel>(size - 1, _input, _name);
            });
        }
    }

    void Controller::loadRouting() {
        auto _mixer = window->objects()[0].as<Gui::Mixer>();
        
        for (auto& _input : Controller::processor.inputs)
            for (auto& _output : _input.output_levels)
                _output = 0;

        std::ifstream _file{ "./routing.txt" };
        for (std::string _str; std::getline(_file, _str);) {
            std::string_view _view = _str;
            if (!_view.contains(":") || _view.starts_with("#")) continue;
            auto _parts = split(_view, ':');
            if (_parts.size() != 2) continue;                 // Routing has 2 parts
            auto _input = trim(_parts[0]);                    // part 1: input
            auto _outputs = trim(_parts[1], " \t\n\r\f\v[]"); // part 2: connected outputs

            int _inputId = -1;
            for (auto& _obj : _mixer->objects()) {
                auto _channel = _obj.as<Gui::Channel>();
                if (_channel->input && _input == _channel->name) {
                    _inputId = _channel->id;
                    break;
                }
            }
            if (_inputId == -1) continue;

            std::vector<std::string_view> _outputsVec = split(_outputs, ',');
            for (auto _name : _outputsVec) {
                _name = trim(_name);

                int _outputId = -1;
                for (auto& _obj : _mixer->objects()) {
                    auto _channel = _obj.as<Gui::Channel>();
                    if (!_channel->input && _name == _channel->name) {
                        _outputId = _channel->id;
                        break;
                    }
                }

                if (_outputId == -1) continue;

                Controller::processor.inputs[_inputId].output_levels[_outputId] = 1;
            }
        }
    }

    void Controller::saveRouting() {
        auto _mixer = window->objects()[0].as<Gui::Mixer>();

        std::ofstream _file{ "./routing.txt" };
        for (auto& _obj : _mixer->objects()) {
            auto _input = _obj.as<Gui::Channel>();
            if (!_input->input) continue;
            _file << _input->name << ":[";
            bool _first = true;
            for (auto& _obj : _mixer->objects()) {
                auto _output = _obj.as<Gui::Channel>();
                if (_output->input) continue;

                auto _level = Controller::processor.inputs[_input->id].output_levels[_output->id];
                if (_level) {
                    if (!_first) _file << ",";
                    _file << _output->name;
                    _first = false;
                }
            }
            _file << "]\n";
        }
    }

    void Controller::start() {
        Guijo::Gui _gui;
        window = _gui.emplace<Window>({
            .name = "Mixijo",
            .dimensions { -1, -1, 500, 500 },
        });

        window->event<[](Window& self, const KeyPress& e) {
            if (e.mod & Mods::Control && e.keycode == 'R') {
                loadChannels();
                loadRouting();
            } else if (e.mod & Mods::Control && e.keycode == 'T') {
                loadTheme();
            } else if (e.mod & Mods::Control && e.keycode == 'D') {
                saveRouting();
                loadDevices();
                loadChannels();
                loadRouting();
            } else if (e.mod & Mods::Control && e.keycode == 'M') {
                Controller::processor.midiin.Close();
                Controller::processor.midiout.Close();
                refreshDeviceNames();
                Controller::processor.initMidi();
            }
        }>();

        window->emplace<Gui::Mixer>();
        loadSettings();

        while (_gui.loop()) {
            processor.midiin.HandleEvents();
        }

        processor.deinit();
    }
}