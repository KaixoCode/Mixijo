#pragma once
#include "pch.hpp"
#include "Gui/Channel.hpp"

namespace Mixijo::Gui {

    struct Mixer : Object {
        Mixer();

        StateLinked<Animated<Color>> background;
        StateLinked<Animated<Color>> divider;
        float dividerX = 0;

        void mouseClick(const MousePress& e);

        void draw(DrawContext& p) const override;
        void update() override;

        void updateTheme();
    };
}