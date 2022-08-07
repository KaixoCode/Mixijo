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

    struct Frame : Window {
        StateLinked<Animated<Color>> border;
        StateLinked<Animated<Color>> title;

        struct Button : Object {
            StateLinked<Animated<Color>> background;
            StateLinked<Animated<Color>> icon;
            std::function<void(void)> callback;
            int type = 0;

            Button(int type);

            void mouseRelease(const MouseRelease& e);
            void draw(DrawContext& p) const override;
        };

        Pointer<Button> close{ emplace<Button>(0) };
        Pointer<Button> maximize{ emplace<Button>(1) };
        Pointer<Button> minimize{ emplace<Button>(2) };
        Pointer<Object> mixer;

        Frame(Window::Construct);

        void draw(DrawContext& p) const override;
        void update() override;

        void updateTheme();
    };

    struct Controller {
        static double sampleRate;
        static std::string audioDevice;
        static std::string midiinDevice;
        static std::string midioutDevice;
        static int selectedChannel;
        static bool selectedInput;
        static Processor processor;
        static Pointer<Frame> window;

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
                ThemeComponent<Color> slider{};
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
                ThemeComponent<Color> icon{};
            } close;

            struct {
                ThemeComponent<Color> background{};
                ThemeComponent<Color> icon{};
            } minimize;

            struct {
                ThemeComponent<Color> background{};
                ThemeComponent<Color> icon{};
            } maximize;

            struct {
                ThemeComponent<Color> background{};
                ThemeComponent<Color> border{};
                ThemeComponent<float> borderWidth{};
            } routebutton;

            ThemeComponent<Color> background{};
            ThemeComponent<Color> border{};
            ThemeComponent<Color> divider{};

            void reset() {
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
        };

        static Theme theme;
    };
}