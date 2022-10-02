#include "Controller.hpp"
#include "Gui/Mixer.hpp"
#include "Gui/Channel.hpp"
#include "Utils.hpp"
#include "resource.h"

namespace Mixijo {
    double Controller::maxDb = 12;
    double Controller::maxLin = std::pow(10, 0.0125 * maxDb);
    double Controller::sampleRate = 48000;
    int Controller::bufferSize = 512;
    std::string Controller::audioDevice{};
    std::string Controller::midiinDevice{};
    std::string Controller::midioutDevice{};
    std::vector<std::pair<int, std::string>> Controller::buttons{};
    int Controller::selectedChannel = -1;
    bool Controller::selectedInput = false;
    bool Controller::showConsole = true;
    Processor Controller::processor{};
    Pointer<Frame> Controller::window{};
    Controller::Theme Controller::theme{};
    std::ofstream Controller::logOutput{};

    void Controller::refreshSettings() {
        std::ifstream _file{ "./settings.json" };
        if (!_file.is_open()) {
            errline("cannot find settings file!");
            return;
        }
        std::string _content{ std::istreambuf_iterator<char>{ _file }, std::istreambuf_iterator<char>{} };

        std::optional<json> _result = json::parse(_content);
        if (!_result.has_value()) {
            errline("cannot parse settings file! Invalid json.");
            return;
        }

        json& _json = _result.value();

        if (_json.contains("audio", json::String)) audioDevice = _json["audio"].as<json::string>();
        if (_json.contains("samplerate", json::Unsigned)) sampleRate = _json["samplerate"].as<json::unsigned_integral>();
        if (_json.contains("buffersize", json::Unsigned)) bufferSize = _json["buffersize"].as<json::unsigned_integral>();
        if (_json.contains("midiin", json::String)) midiinDevice = _json["midiin"].as<json::string>();
        if (_json.contains("midiout", json::String)) midioutDevice = _json["midiout"].as<json::string>();
        if (_json.contains("buttons", json::Array)) {
            buttons.clear();
            for (auto& _link : _json["buttons"].as<json::array>()) {
                if (_link.contains("cc", json::Unsigned) && _link.contains("run", json::String)) {
                    buttons.push_back({
                        (int)_link["cc"].as<json::unsigned_integral>(),
                        _link["run"].as<json::string>() 
                    });
                }
            }
        }
        if (_json.contains("channels", json::Object)) {
            auto _mixer = window->mixer.as<Gui::Mixer>();
            _mixer->objects().clear();
            processor.access([](Processor::Inputs& in, Processor::Outputs& out) {
                in.clear();
                out.clear();
            });

            auto _addChannel = [&](json& channel, bool input) {
                processor.access([&](Processor::Inputs& in, Processor::Outputs& out) {
                    auto& _channel = input ? (Channel&) in.add() : out.add();

                    if (channel.contains("endpoints")) {
                        if (channel["endpoints"].is(json::Array)) {
                            auto& _endpoints = channel["endpoints"].as<json::array>();
                            for (auto& _endpoint : _endpoints) if (_endpoint.is(json::String)) {
                                int _id = processor.find_endpoint(_endpoint.as<json::string>(), input);
                                if (_id != -1) _channel.add(_id);
                                else {
                                    errline("could not find endpoint \"", _endpoint.as<json::string>(), "\"");
                                }
                            } else {
                                errline("endpoint should be a string.");
                            }
                        } else {
                            errline("endpoints should be an array of strings.");
                        }
                    }

                    if (channel.contains("midimapping")) {
                        if (channel["midimapping"].is(json::Array)) {
                            auto& _mapping = channel["midimapping"].as<json::array>();

                            for (auto& _map : _mapping) {
                                if (!_map.is(json::Object)) {
                                    errline("failed to parse a midi mapping, should be a json object like this:");
                                    errline("  { \"cc\": 73, \"param\" : \"gain\" }");
                                    continue;
                                }
                                if (_map.contains("cc", json::Unsigned) && _map.contains("param", json::String)) {
                                    _channel.addMidiLink(_map["param"].as<json::string>(), _map["cc"].as<json::unsigned_integral>());
                                } else {
                                    errline("failed to parse a midi mapping, should be a json object like this:");
                                    errline("  { \"cc\": 73, \"param\" : \"gain\" }");
                                }
                            }
                        } else {
                            errline("midimapping should be an array of json objects.");
                        }
                    }

                    int _size = input ? in.size() : out.size();
                    if (channel.contains("name", json::String))
                        _mixer->emplace<Gui::Channel>(_size - 1, input, channel["name"].as<json::string>());
                    else _mixer->emplace<Gui::Channel>(_size - 1, input, "channel");
                });
            };

            auto& _channels = _json["channels"];
            if (_channels.contains("outputs", json::Array))
                for (auto& _channel : _channels["outputs"].as<json::array>())
                    _addChannel(_channel, false);
            if (_channels.contains("inputs", json::Array))
                for (auto& _channel : _channels["inputs"].as<json::array>())
                    _addChannel(_channel, true);
        }
        if (_json.contains("theme", json::Object)) {
            theme.reset();
            auto& _theme = _json["theme"];
            logline("loading theme...");
            constexpr auto _decodeColor = [](json& j, auto name) -> Color {
                if (!j.is(json::Array)) {
                    errline("failed to parse \"", name, "\", expecting json array.");
                    return Color{};
                }
                for (auto& v : j.as<json::array>())
                    if (!v.is(json::Unsigned)) {
                        errline("failed to parse \"", name, "\", expecting json array with 1 to 4 unsigned integrals.");
                        return Color{};
                    }
                if (j.size() == 1) return Color{
                    static_cast<float>(j[0].as<json::unsigned_integral>())
                };
                else if (j.size() == 2) return Color{
                    static_cast<float>(j[0].as<json::unsigned_integral>()),
                    static_cast<float>(j[1].as<json::unsigned_integral>()),
                };
                else if (j.size() == 3) return Color{
                    static_cast<float>(j[0].as<json::unsigned_integral>()),
                    static_cast<float>(j[1].as<json::unsigned_integral>()),
                    static_cast<float>(j[2].as<json::unsigned_integral>()),
                };
                else if (j.size() == 4) return Color{
                    static_cast<float>(j[0].as<json::unsigned_integral>()),
                    static_cast<float>(j[1].as<json::unsigned_integral>()),
                    static_cast<float>(j[2].as<json::unsigned_integral>()),
                    static_cast<float>(j[3].as<json::unsigned_integral>()),
                }; 
                else {
                    errline("failed to parse \"", name, "\", expecting json array with 1 to 4 unsigned integrals.");
                    return Color{};
                }
            };

            constexpr auto _giveColor = [](auto& t, json& j, auto name) {
                if (j.is(json::Array)) t.value.emplace(_decodeColor(j, std::string{ name } + ".color"));
                else if (j.is(json::Object)) {
                    if (j.contains("transition")) {
                        if (j["transition"].is(json::Unsigned)) {
                            t.transition = j["transition"].as<json::unsigned_integral>();
                        } else {
                            errline("failed to parse \"", name, ".transition\", expecting unsigned integral.");
                        }
                    }
                    if (j.contains("color")) {
                        if (j["color"].is(json::Array)) {
                            t.value.emplace(_decodeColor(j["color"], std::string{ name } + ".color"));
                        } else {
                            errline("failed to parse \"", name, ".color\", expecting json array.");
                        }
                    }
                    for (auto& [_key, _value] : j.as<json::object>()) {
                        if (_key == "disabled") t.statelinked.push_back({ Disabled, _decodeColor(j["disabled"], std::string{ name } + ".disabled") });
                        if (_key == "hovering") t.statelinked.push_back({ Hovering, _decodeColor(j["hovering"], std::string{ name } + ".hovering") });
                        if (_key == "selected") t.statelinked.push_back({ Selected, _decodeColor(j["selected"], std::string{ name } + ".selected") });
                        if (_key == "pressed") t.statelinked.push_back({ Pressed, _decodeColor(j["pressed"], std::string{ name } + ".pressed") });
                        if (_key == "focused") t.statelinked.push_back({ Focused, _decodeColor(j["focused"], std::string{ name } + ".focused") });
                    }
                } else {
                    errline("failed to parse Color \"", name, "\", expecting json array or object");
                }
            };
            
            constexpr auto _giveValue = [](auto& t, json& j, auto name) {
                if (j.is(json::Unsigned)) t.value.emplace(j.as<json::unsigned_integral>());
                else if (j.is(json::Object)) {
                    if (j.contains("transition")) {
                        if (j["transition"].is(json::Unsigned)) {
                            t.transition = j["transition"].as<json::unsigned_integral>();
                        } else {
                            errline("failed to parse \"", name, ".transition\", expecting unsigned integral.");
                        }
                    }
                    if (j.contains("value")) {
                        if (j["value"].is(json::Unsigned)) {
                            t.value.emplace(j["value"].as<json::unsigned_integral>());
                        } else {
                            errline("failed to parse \"", name, ".value\", expecting unsigned integral.");
                        }
                    }
                    for (auto& [_key, _value] : j.as<json::object>()) {
                        if (_key == "disabled") {
                            if (_value.type() == json::Unsigned)
                                t.statelinked.push_back({ Disabled, j["disabled"].as<json::unsigned_integral>() });
                            else errline("failed to parse \"", name, ".disabled\", expecting unsigned integral.");
                        }
                        if (_key == "hovering") {
                            if (_value.type() == json::Unsigned)
                                t.statelinked.push_back({ Hovering, j["hovering"].as<json::unsigned_integral>() });
                            else errline("failed to parse \"", name, ".hovering\", expecting unsigned integral.");
                        }
                        if (_key == "selected") {
                            if (_value.type() == json::Unsigned)
                                t.statelinked.push_back({ Selected, j["selected"].as<json::unsigned_integral>() });
                            else errline("failed to parse \"", name, ".selected\", expecting unsigned integral.");
                        }
                        if (_key == "pressed") {
                            if (_value.type() == json::Unsigned)
                                t.statelinked.push_back({ Pressed, j["pressed"].as<json::unsigned_integral>() });
                            else errline("failed to parse \"", name, ".pressed\", expecting unsigned integral.");
                        }
                        if (_key == "focused") {
                            if (_value.type() == json::Unsigned)
                                t.statelinked.push_back({ Focused, j["focused"].as<json::unsigned_integral>() });
                            else errline("failed to parse \"", name, ".focused\", expecting unsigned integral.");
                        }
                    }
                } else {
                    errline("failed to parse number \"", name, "\", expecting unsigned integer or object");
                }
            };

            if (_theme.contains("background")) _giveColor(theme.background, _theme["background"], "theme.background");
            if (_theme.contains("divider")) _giveColor(theme.divider, _theme["divider"], "theme.divider");
            if (_theme.contains("border")) _giveColor(theme.border, _theme["border"], "theme.border");
            if (_theme.contains("channel")) {
                auto& _channel = _theme["channel"];
                if (_channel.contains("background")) _giveColor(theme.channel.background, _channel["background"], "theme.channel.background");
                if (_channel.contains("slider")) _giveColor(theme.channel.slider, _channel["slider"], "theme.channel.slider");
                if (_channel.contains("meter")) {
                    auto& _meter = _channel["meter"];
                    _giveColor(theme.channel.meter, _meter, "meter");
                    if (_meter.contains("text")) _giveColor(theme.channel.meterText, _meter["text"], "theme.channel.meter.text");
                    if (_meter.contains("line1")) _giveColor(theme.channel.meterLine1, _meter["line1"], "theme.channel.meter.line1");
                    if (_meter.contains("line2")) _giveColor(theme.channel.meterLine2, _meter["line2"], "theme.channel.meter.line2");
                    if (_meter.contains("background")) _giveColor(theme.channel.meterBackground, _meter["background"], "theme.channel.meter.background");
                }
                if (_channel.contains("border")) {
                    auto& _border = _channel["border"];
                    _giveColor(theme.channel.border, _border, "theme.channel.border");
                    if (_border.contains("width")) _giveValue(theme.channel.borderWidth, _border["width"], "theme.channel.border.width");
                }
                if (_channel.contains("title")) _giveColor(theme.channel.title, _channel["title"], "theme.channel.title");
                if (_channel.contains("value")) _giveColor(theme.channel.value, _channel["value"], "theme.channel.value");
            }
            if (_theme.contains("routebutton")) {
                auto& _rbttn = _theme["routebutton"];
                if (_rbttn.contains("background")) _giveColor(theme.routebutton.background, _rbttn["background"], "theme.routebutton.background");
                if (_rbttn.contains("border")) {
                    auto& _border = _rbttn["border"];
                    _giveColor(theme.routebutton.border, _border, "theme.routebutton.border");
                    if (_border.contains("width")) _giveValue(theme.routebutton.borderWidth, _border["width"], "theme.routebutton.border.width");
                }
            }
            if (_theme.contains("close")) {
                auto& _close = _theme["close"];
                if (_close.contains("background")) _giveColor(theme.close.background, _close["background"], "theme.close.background");
                if (_close.contains("icon")) _giveColor(theme.close.icon, _close["icon"], "theme.close.icon");
            }
            if (_theme.contains("minimize")) {
                auto& _minimize = _theme["minimize"];
                if (_minimize.contains("background")) _giveColor(theme.minimize.background, _minimize["background"], "theme.minimize.background");
                if (_minimize.contains("icon")) _giveColor(theme.minimize.icon, _minimize["icon"], "theme.minimize.icon");
            }
            if (_theme.contains("maximize")) {
                auto& _maximize = _theme["maximize"];
                if (_maximize.contains("background")) _giveColor(theme.maximize.background, _maximize["background"], "theme.maximize.background");
                if (_maximize.contains("icon")) _giveColor(theme.maximize.icon, _maximize["icon"], "theme.maximize.icon");
            }
            logline("loaded theme");
            window->updateTheme(); // Update theme in gui
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
        std::filesystem::path _logpath = "logs/";
        if (!std::filesystem::exists(_logpath))
            std::filesystem::create_directory("logs/");
        logOutput.open(std::format("logs/mixijo_{:%EY%Om%Od_%OH%OM%OS}.log", std::chrono::system_clock::now()));

        Guijo::Gui _gui;
        window = _gui.emplace<Frame>(Window::Construct{
            .name = "Mixijo",
            .dimensions { -1, -1, 1000, 500 },
        });

        window->setIcon(IDI_ICON1);

        window->event<[](Window& self, const KeyPress& e) {
            if (e.mod & Mods::Control && e.keycode == 'S') {
                logline("Saving routing...");
                saveRouting();
                logline("Saved routing");
            } else if (e.mod & Mods::Control && e.mod & Mods::Shift && e.keycode == 'R') {
                logline("Reloading settings and reopening devices...");
                saveRouting();
                refreshSettings();
                processor.init();
                loadRouting();
                logline("Reloaded settings and reopened devices");
            } else if (e.mod & Mods::Control && e.keycode == 'R') {
                logline("Reloading settings...");
                saveRouting();
                refreshSettings();
                loadRouting();
                logline("Reloaded settings");
            } else if (e.mod & Mods::Control && e.keycode == 'C') {
                if (Controller::showConsole) {
                    Controller::showConsole = false;
                    logline("Hiding console window");
                    ShowWindow(GetConsoleWindow(), SW_HIDE);
                } else {
                    Controller::showConsole = true;
                    logline("Showing console window");
                    ShowWindow(GetConsoleWindow(), SW_SHOW);
                }
            } else if (e.mod & Mods::Control && e.keycode == 'I') {
                logline("Opening ASIO Control Panel");
                Controller::processor.OpenControlPanel();
            } else if (e.mod & Mods::Control && e.keycode == 'L') {
                logline("===========================================");
                logline("               Information                 ");
                logline("===========================================");
                if (Controller::processor.Information().state == Audijo::StreamState::Closed) {
                    logline("no audio device opened, available devices:");
                    for (auto& device : Controller::processor.Devices()) {
                        logline("  ", device.name);
                    }
                } else {
                    auto& _channels = Controller::processor.endpoints();
                    logline("available audio devices:");
                    for (auto& device : Controller::processor.Devices()) {
                        logline("  ", device.name);
                    }
                    logline("opened audio device: ", Controller::audioDevice);
                    logline("  inputs:");
                    for (auto& _channel : _channels) {
                        if (_channel.input) logline("    ", _channel.name);
                    }
                    logline("  outputs:");
                    for (auto& _channel : _channels) {
                        if (!_channel.input) logline("    ", _channel.name);
                    }
                }

                logline("available midi-in devices:");
                for (auto& _channel : Controller::processor.midiin.Devices()) {
                    logline("  ", _channel.name);
                }
                logline("opened midi-in device: ", Controller::midiinDevice);

                logline("available midi-out devices:");
                for (auto& _channel : Controller::processor.midiout.Devices()) {
                    logline("  ", _channel.name);
                }
                logline("opened midi-out device: ", Controller::midioutDevice);

                logline("buffersize: ", Controller::bufferSize);
                logline("sampleRate: ", Controller::sampleRate);
                if (!buttons.empty()) {
                    logline("Buttons: ");
                    for (auto& _button : buttons) {
                        logline("  ", _button.first, " -> ", _button.second);
                    }
                }
                logline("===========================================");
            }
        }>();

        refreshSettings();
        processor.init();
        refreshSettings();
        loadRouting();

        processor.midiin.Callback([&](const Midijo::CC& e) {
            if (e.Value() == 0) return;
            for (auto& _link : buttons) {
                if (_link.first == e.Number()) {
                    logline("Attempted to run file (", _link.second, ")");
                    std::string _command = "start " + _link.second;
                    std::system(_command.c_str());
                }
            }
        });

        while (_gui.loop()) {
            processor.midiin.HandleEvents();
        }

        saveRouting();
        processor.deinit();
    }

    void Controller::Theme::reset() {
        background.reset();
        divider.reset();
        border.reset();
        close.background.reset();
        close.icon.reset();
        minimize.background.reset();
        minimize.icon.reset();
        maximize.background.reset();
        maximize.icon.reset();
        routebutton.background.reset();
        routebutton.border.reset();
        routebutton.borderWidth.reset();
        channel.background.reset();
        channel.slider.reset();
        channel.meter.reset();
        channel.meterBackground.reset();
        channel.border.reset();
        channel.meterLine1.reset();
        channel.meterLine2.reset();
        channel.meterText.reset();
        channel.title.reset();
        channel.value.reset();
        channel.borderWidth.reset();
    }
}