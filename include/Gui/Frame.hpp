#pragma once 
#include "pch.hpp"

namespace Mixijo {
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
        bool initialResize = false;

        Frame(Window::Construct);

        void draw(DrawContext& p) const override;
        void update() override;

        void updateTheme();
    };
}