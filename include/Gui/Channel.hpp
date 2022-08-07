#pragma once
#include "pch.hpp"
#include "Utils.hpp"
#include "Gui/RouteButton.hpp"

namespace Mixijo::Gui {
    struct Channel : Object {
        int id;
        bool input;
        std::string name;
        std::string gain = "";
        std::vector<double> smoothed{};

        StateLinked<Animated<Color>> background{};
        StateLinked<Animated<Color>> meter{};
        StateLinked<Animated<Color>> meterBackground{};
        StateLinked<Animated<Color>> border{};
        StateLinked<Animated<Color>> meterLine1{};
        StateLinked<Animated<Color>> meterLine2{};
        StateLinked<Animated<Color>> meterText{};
        StateLinked<Animated<Color>> title{};
        StateLinked<Animated<Color>> value{};
        StateLinked<Animated<float>> borderWidth{};

        Pointer<RouteButton> route;

        Channel(int gid, bool in, std::string_view name);

        void draw(DrawContext& p) const override;
        void update() override;

        void updateTheme();
    };
}