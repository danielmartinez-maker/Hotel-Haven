#include "hh/frontend/UiSettingsCodec.h"

#include <sstream>
#include <string>

namespace hh::frontend {

std::string serializeUiSettings(const UiSettings& settings) {
    std::ostringstream out;
    out << "HHUI1 " << settings.scalePercent() << ' '
        << (settings.reducedMotion() ? 1 : 0);
    for (const int keyCode : settings.keyboardBindings())
        out << ' ' << keyCode;
    out << '\n';
    return out.str();
}

bool deserializeUiSettings(std::string_view encoded, UiSettings& settings) {
    std::istringstream input{std::string(encoded)};
    std::string magic;
    int scale = 0;
    int reducedMotion = 0;
    UiSettings::KeyboardBindings bindings{};

    if (!(input >> magic >> scale >> reducedMotion) || magic != "HHUI1")
        return false;
    if (reducedMotion != 0 && reducedMotion != 1)
        return false;
    for (int& keyCode : bindings) {
        if (!(input >> keyCode))
            return false;
    }

    std::string trailing;
    if (input >> trailing)
        return false;

    UiSettings candidate = settings;
    if (!candidate.setScalePercent(scale) ||
        !candidate.setKeyboardBindings(bindings))
        return false;
    candidate.setReducedMotion(reducedMotion != 0);
    settings = candidate;
    return true;
}

} // namespace hh::frontend
