#include "d3d11/D3D11Renderer.h"

#include <iostream>

int main() {
    std::cout << "ASAN_D3D_MAIN_REACHED" << std::endl;
    hh::renderer::D3D11Renderer renderer;
    renderer.shutdown();
    std::cout << "ASAN_D3D_RENDERER_OBJECT_OK" << std::endl;
    return 0;
}
