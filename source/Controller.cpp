#include "Controller.hpp"
#include "Gui/Mixer.hpp"
#include "Gui/Channel.hpp"
#include "Utils.hpp"

namespace Mixijo {

    Frame::Button::Button(int type) : type(type) {
        background = Color{ 120, 120, 120, 0 };
        background[Pressed] = Color{ 120, 120, 120, 150 };
        background[Hovering] = Color{ 120, 120, 120, 90 };
        icon = Color{ 250, 250, 250 };
        link(background);
        link(icon);
        event<Button>();
    }

    void Frame::Button::mouseRelease(const MouseRelease& e) {
        if (hitbox(e.pos) && callback) callback();
    }

    void Frame::Button::draw(DrawContext& p) const {
        p.strokeWeight(0);
        p.fill(background);
        p.rect(dimensions());
        p.fill({ 0, 0, 0, 0 });
        p.strokeWeight(1);
        p.stroke(icon);
        auto _center = dimensions().center();
        auto _size = 5;
        switch (type) {
        case 0:
            p.line({ _center.x() - _size, _center.y() - _size }, { _center.x() + _size, _center.y() + _size });
            p.line({ _center.x() - _size, _center.y() + _size }, { _center.x() + _size, _center.y() - _size });
            break;
        case 1:
            p.rect(Dimensions{ _center.x() - _size - 0.5, _center.y() - _size - 0.5, _size * 2, _size * 2 });
            break;
        case 2:
            p.line({ _center.x() - _size, _center.y() }, { _center.x() + _size, _center.y() });
            break;
        }
    }

    Frame::Frame(Window::Construct c) 
        : Window(c) 
    {
        title = Color{ 200, 200, 200 };
        mixer = emplace<Gui::Mixer>().as<Object>();
        box.use = false;

        close->callback = [&] {
            m_ShouldExit = true;
        };

        maximize->callback = [&] {
            if (IsMaximized(m_Handle)) ShowWindow(m_Handle, SW_RESTORE);
            else ShowWindow(m_Handle, SW_MAXIMIZE);
        };

        minimize->callback = [&] {
            ShowWindow(m_Handle, SW_MINIMIZE);
        };
    }

    void Frame::draw(DrawContext& p) const {
        p.strokeWeight(0);
        p.fill(border);
        p.rect(Dimensions{ 0, 0, width(), height() });
        Object::draw(p);
    }

    void Frame::update() {
        if (IsMaximized(m_Handle)) {
            mixer->dimensions({ 16, 40, width() - 32, height() - 56 });
            close->dimensions({ width() - 45 * 1 - 8, 8, 45, 29 });
            maximize->dimensions({ width() - 45 * 2 - 8, 8, 45, 29 });
            minimize->dimensions({ width() - 45 * 3 - 8, 8, 45, 29 });
        } else {
            mixer->dimensions({ 8, 32, width() - 16, height() - 40 });
            close->dimensions({ width() - 45 * 1, 0, 45, 29 });
            maximize->dimensions({ width() - 45 * 2, 0, 45, 29 });
            minimize->dimensions({ width() - 45 * 3, 0, 45, 29 });
        }
        Window::update();
    }

    void Frame::updateTheme() {
        Controller::theme.border.assign(border);
        Controller::theme.close.background.assign(close->background);
        Controller::theme.close.icon.assign(close->icon);
        Controller::theme.minimize.background.assign(minimize->background);
        Controller::theme.minimize.icon.assign(minimize->icon);
        Controller::theme.maximize.background.assign(maximize->background);
        Controller::theme.maximize.icon.assign(maximize->icon);
        mixer.as<Gui::Mixer>()->updateTheme();
    }

    double Controller::maxDb = 12;
    double Controller::maxLin = std::pow(10, 0.0125 * maxDb);
    double Controller::sampleRate = 48000;
    int Controller::bufferSize = 512;
    std::string Controller::audioDevice;
    std::string Controller::midiinDevice;
    std::string Controller::midioutDevice;
    std::vector<std::pair<int, std::string>> Controller::buttons{};
    int Controller::selectedChannel = -1;
    bool Controller::selectedInput = false;
    Processor Controller::processor{};
    Pointer<Frame> Controller::window{};
    Controller::Theme Controller::theme;

    void Controller::loadSettings() {
        loadDevices();
        loadChannels();
        loadTheme();
        loadRouting();
    }

    void Controller::refreshSettings() {
        std::ifstream _file{ "./settings.txt" };
        if (!_file.is_open()) return;
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
            if (_device == "samplerate") sampleRate = parse<double>(_name);
            if (_device == "buffersize") bufferSize = parse<int>(_name);
            if (_device == "maxdb") maxDb = parse<double>(_name), maxLin = std::pow(10, 0.0125 * maxDb);
            if (_device == "buttons") {
                auto _list = trim(_name, "[]");
                auto _links = split(_list, ',');

                buttons.clear();

                for (auto& _link : _links) {
                    auto _parts = split(_link, ';');
                    if (_parts.size() != 2) continue;
                    auto _midiCC = parse<int>(trim(_parts[0]));
                    auto _file = trim(_parts[1]);
                    buttons.emplace_back(_midiCC, _file);
                }
            }
        }
    }

    void Controller::loadDevices() {
        processor.deinit();
        refreshSettings();
        processor.init();
    }

    void Controller::loadTheme() {
        std::ifstream _file{ "./theme.txt" };
        if (!_file.is_open()) return;
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

            if (_tryValue("close.background", theme.close.background)) continue;
            if (_tryValue("close.icon", theme.close.icon)) continue;
            if (_tryValue("minimize.background", theme.minimize.background)) continue;
            if (_tryValue("minimize.icon", theme.minimize.icon)) continue;
            if (_tryValue("maximize.background", theme.maximize.background)) continue;
            if (_tryValue("maximize.icon", theme.maximize.icon)) continue;
            if (_tryValue("border", theme.border)) continue;
            if (_tryValue("divider", theme.divider)) continue;
            if (_tryValue("background", theme.background)) continue;
            if (_tryValue("routebutton.background", theme.routebutton.background)) continue;
            if (_tryValue("routebutton.borderWidth", theme.routebutton.borderWidth)) continue;
            if (_tryValue("routebutton.border", theme.routebutton.border)) continue;
            if (_tryValue("channel.background", theme.channel.background)) continue;
            if (_tryValue("channel.slider", theme.channel.slider)) continue;
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

        window->updateTheme(); // Update theme in gui
    }

    void Controller::loadChannels() {
        // Get gui mixer object
        auto _mixer = window->mixer.as<Gui::Mixer>();
        _mixer->objects().clear(); 
        processor.access([](Processor::Inputs& in, Processor::Outputs& out) {
            in.clear();
            out.clear();
        });

        // Load channel settings
        std::ifstream _file{ "./channels.txt" };
        if (!_file.is_open()) return;
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
        auto _mixer = window->mixer.as<Gui::Mixer>();
        
        for (auto& _input : Controller::processor.inputs)
            for (auto& _output : _input.output_levels)
                _output = 0;

        std::ifstream _file{ "./routing.txt" };
        if (!_file.is_open()) return;
        for (std::string _str; std::getline(_file, _str);) {
            std::string_view _view = _str;
            if (!_view.contains(":") || _view.starts_with("#")) continue;
            auto _parts = split(_view, ':');
            if (_parts.size() < 2) continue;
            auto _isInput = _parts.size() == 3;
            auto _channelName = trim(_parts[0]);                     // part 1: input
            auto _settings = trim(_parts[1], " \t\n\r\f\v[]"); // part 2: channel settings

            int _channelId = -1;
            for (auto& _obj : _mixer->objects()) {
                auto _channel = _obj.as<Gui::Channel>();
                if (_channel->input == _isInput && _channelName == _channel->name) {
                    _channelId = _channel->id;
                    break;
                }
            }
            if (_channelId == -1) continue;

            std::vector<std::string_view> _settingsVec = split(_settings, ',');
            for (auto _setting : _settingsVec) {
                _setting = trim(_setting);
                auto _parts = split(_setting, '=');
                if (_parts.size() < 2) continue; // Setting is 'name=val'
                auto _name = trim(_parts[0]);
                double _value = parse<double>(trim(_parts[1]));
                auto& _c = _isInput
                    ? static_cast<Channel&>(Controller::processor.inputs[_channelId])
                    : static_cast<Channel&>(Controller::processor.outputs[_channelId]);
                _c.setSetting(_name, _value);
            }

            if (_parts.size() == 3) {
                auto _outputs = trim(_parts[2], " \t\n\r\f\v[]");  // part 3: connected outputs
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

                    Controller::processor.inputs[_channelId].output_levels[_outputId] = 1;
                }
            }
        }
    }

    void Controller::saveRouting() {
        auto _mixer = window->mixer.as<Gui::Mixer>();

        std::ofstream _file{ "./routing.txt" };
        for (auto& _obj : _mixer->objects()) {
            auto _channel = _obj.as<Gui::Channel>();

            auto& _c = _channel->input
                ? static_cast<Channel&>(Controller::processor.inputs[_channel->id])
                : static_cast<Channel&>(Controller::processor.outputs[_channel->id]);

            _file << _channel->name;
            _file << ":[";
            _c.getSettings(_file);

            if (_channel->input) {
                _file << "]:[";
                bool _first = true;
                for (auto& _obj : _mixer->objects()) {
                    auto _output = _obj.as<Gui::Channel>();
                    if (_output->input) continue;

                    auto _level = Controller::processor.inputs[_channel->id].output_levels[_output->id];
                    if (_level) {
                        if (!_first) _file << ",";
                        _file << _output->name;
                        _first = false;
                    }
                }
            }
            _file << "]\n";
        }
    }

    void Controller::start() {
        Guijo::Gui _gui;
        window = _gui.emplace<Frame>(Window::Construct{
            .name = "Mixijo",
            .dimensions { -1, -1, 1000, 500 },
        });

        window->event<[](Window& self, const KeyPress& e) {
            if (e.mod & Mods::Control && e.keycode == 'R') {
                loadChannels();
                loadRouting();
                std::cout << "Reloaded channels and routing\n";
            } else if (e.mod & Mods::Control && e.keycode == 'T') {
                loadTheme();
                std::cout << "Reloaded theme\n";
            } else if (e.mod & Mods::Control && e.keycode == 'S') {
                saveRouting();
                std::cout << "Saved routing\n";
            } else if (e.mod & Mods::Control && e.keycode == 'P') {
                refreshSettings();
                if (Controller::processor.SetSampleRate(Controller::sampleRate) != Audijo::NoError)
                    std::cout << "Failed to set samplerate to " << Controller::sampleRate << "\n";
                else std::cout << "Set new samplerate to " << Controller::sampleRate << "\n";
            } else if (e.mod & Mods::Control && e.keycode == 'O') {
                refreshSettings();
                std::cout << "Refreshed settings\n";
            } else if (e.mod & Mods::Control && e.keycode == 'D') {
                saveRouting();
                loadDevices();
                loadChannels();
                loadRouting();
                std::cout << "Reopened devices\n";
            } else if (e.mod & Mods::Control && e.keycode == 'I') {
                std::cout << "Opening control panel\n";
                Controller::processor.OpenControlPanel();
            } else if (e.mod & Mods::Control && e.keycode == 'M') {
                Controller::processor.midiin.Close();
                Controller::processor.midiout.Close();
                refreshSettings();
                Controller::processor.initMidi();
                std::cout << "Reopened midi devices\n";
            } else if (e.mod & Mods::Control && e.keycode == 'L') {
                if (Controller::processor.Information().state == Audijo::StreamState::Closed) {
                    std::cout << "no audio device opened\n";
                    std::cout << "available devices:\n";
                    for (auto& device : Controller::processor.Devices(true)) {
                        std::cout << "  " + device.name << "\n";
                    }
                    return;
                }
                auto& _channels = Controller::processor.endpoints();
                for (auto& _channel : _channels) {
                    std::cout << _channel.name << ": " << (_channel.input ? "input" : "output") << "\n";
                }
                std::cout << "audio device: " << Controller::audioDevice << "\n";
                std::cout << "midiin device: " << Controller::midiinDevice << "\n";
                std::cout << "midiout device: " << Controller::midioutDevice << "\n";
                std::cout << "buffersize: " << Controller::bufferSize << "\n";
                std::cout << "sampleRate: " << Controller::sampleRate << "\n";
                std::cout << "maxdb: " << Controller::maxDb << "\n";
                std::cout << "Buttons: [";
                for (auto& _button : buttons) {
                    std::cout << _button.first << ";" << _button.second << ",";
                }
                std::cout << "]\n";
            }
        }>();

        loadSettings();

        processor.midiin.Callback([&](const Midijo::CC& e) {
            if (e.Value() == 0) return;
            for (auto& _link : buttons) {
                if (_link.first == e.Number()) {
                    std::cout << "Attempted to run batch file (" << _link.second << ")\n";
                    std::string _command = "start " + _link.second;
                    std::system(_command.c_str());
                }
            }
        });

        while (_gui.loop()) {
            processor.midiin.HandleEvents();
        }

        processor.deinit();
    }
}