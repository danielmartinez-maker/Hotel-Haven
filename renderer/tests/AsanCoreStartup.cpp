#include "hh/renderer/Camera.h"

#include <iostream>

int main() {
    std::cout << "ASAN_CORE_MAIN_REACHED" << std::endl;
    hh::renderer::OrthoCamera camera;
    camera.setTarget({1.0f, 2.0f, 3.0f});
    const auto position = camera.worldPosition();
    std::cout << "ASAN_CORE_CAMERA_OK " << position.x << ' ' << position.y << ' ' << position.z << std::endl;
    return 0;
}
