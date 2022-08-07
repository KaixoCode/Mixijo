#pragma once
#include "pch.hpp"

namespace Mixijo::Gui {
    struct RouteButton : Object {
        StateLinked<Animated<Color>> background{};
        StateLinked<Animated<Color>> border{};
        StateLinked<Animated<float>> borderWidth{};
        std::function<void(void)> callback;

        RouteButton();

        void mousePress(const MousePress& e);
        void draw(DrawContext& p) const override;

        void updateTheme();
    };
}