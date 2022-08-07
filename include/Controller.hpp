#pragma once
#include "pch.hpp"
#include "Processing/Processor.hpp"

namespace Mixijo {

    template<class Ty>
    struct ThemeComponent {
        using type = Ty;
        void reset() { value.reset(); statelinked.clear(); transition = -1; }

        float transition = -1;
        std::optional<Ty> value{};
        std::vector<std::pair<StateId, Ty>> statelinked{};

        void assign(StateLinked<Animated<Ty>>& v) {
            v.clear();
            if (value.has_value()) v = value.value();
            for (auto& [_id, _value] : statelinked) v[_id] = _value;
            if (transition != -1) v.transition(transition);
        }
    };

    struct Controller {
        static std::string audioDevice;
        static std::string midiinDevice;
        static std::string midioutDevice;
        static int selectedChannel;
        static bool selectedInput;
        static Processor processor;
        static Pointer<Window> window;

        static void refreshDeviceNames();
        static void loadSettings();
        static void loadChannels();
        static void loadTheme();
        static void loadRouting();
        static void loadDevices();
        static void saveRouting();
        static void start();

        struct Theme {
            struct {
                ThemeComponent<Color> background{};
                ThemeComponent<Color> meter{};
                ThemeComponent<Color> meterBackground{};
                ThemeComponent<Color> border{};
                ThemeComponent<Color> meterLine1{};
                ThemeComponent<Color> meterLine2{};
                ThemeComponent<Color> meterText{};
                ThemeComponent<Color> title{};
                ThemeComponent<Color> value{};
                ThemeComponent<float> borderWidth{};
            } channel;

            struct {
                ThemeComponent<Color> background{};
                ThemeComponent<Color> border{};
                ThemeComponent<float> borderWidth{};
            } routebutton;

            ThemeComponent<Color> background{};
            ThemeComponent<Color> divider{};

            void reset() {
                background.reset();
                divider.reset();
                routebutton.background.reset();
                routebutton.border.reset();
                routebutton.borderWidth.reset();
                channel.background.reset();
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
        };

        static Theme theme;
    };
}