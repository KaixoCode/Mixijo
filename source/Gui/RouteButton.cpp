#include "Gui/RouteButton.hpp"
#include "Controller.hpp"

namespace Mixijo::Gui {
    RouteButton::RouteButton() {
        box.use = false;
        background = { 30, 30, 30 };
        background[Hovering] = { 40, 40, 40 };
        background[Pressed] = { 45, 45, 45 };
        background[Selected] = { 45, 45, 45 };
        background.transition(100);
        borderWidth = 0;
        border = { 0, 0, 0 };
        link(background);
        link(border);
        link(borderWidth);

        event<RouteButton>();
        updateTheme();
    }

    void RouteButton::mousePress(const MousePress& e) {
        if (hitbox(e.pos) && e.button & MouseButton::Left) {
            if (callback) callback();
        }
    }

    void RouteButton::draw(DrawContext& p) const {
        p.stroke(border);
        p.strokeWeight(borderWidth);
        p.fill(background);
        p.rect(dimensions());
        Object::draw(p);
    }

    void RouteButton::updateTheme() {
        Controller::theme.routebutton.background.assign(background);
        Controller::theme.routebutton.border.assign(border);
        Controller::theme.routebutton.borderWidth.assign(borderWidth);
    }
}