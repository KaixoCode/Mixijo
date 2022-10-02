#pragma once
#include "pch.hpp"
#include "Processing/Processor.hpp"
#include "Gui/Frame.hpp"

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
        static double maxDb;
        static double maxLin;
        static double sampleRate;
        static int bufferSize;
        static std::string audioDevice;
        static std::string midiinDevice;
        static std::string midioutDevice;
        static std::vector<std::pair<int, std::string>> buttons;
        static int selectedChannel;
        static bool selectedInput;
        static Processor processor;
        static Pointer<Frame> window;
        static std::ofstream logOutput;

        static void refreshSettings();
        static void loadRouting();
        static void saveRouting();
        static void start();

        template<class ...Args>
        static void log(Args&&... args) {
            const auto now = std::chrono::system_clock::now();
            std::string prefix = std::format("[Mixijo] {:%EY-%Om-%Od %OH:%OM:%OS}: ", now);
            std::cout << prefix;
            ((std::cout << args), ...);
            if (logOutput.is_open()) {
                logOutput << prefix;
                ((logOutput << args), ...);
            }
        }
        
        template<class ...Args>
        static void logline(Args&&... args) {
            const auto now = std::chrono::system_clock::now();
            std::string prefix = std::format("[Mixijo] {:%EY-%Om-%Od %OH:%OM:%OS}: ", now);
            std::cout << prefix;
            ((std::cout << args), ...);
            std::cout << '\n';
            if (logOutput.is_open()) {
                logOutput << prefix;
                ((logOutput << args), ...);
                logOutput << '\n';
            }
        }
        
        template<class ...Args>
        static void logPart(Args&&... args) {
            ((std::cout << args), ...);
            if (logOutput.is_open()) {
                ((logOutput << args), ...);
            }
        }

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

            void reset();
        };

        static Theme theme;
    };
}