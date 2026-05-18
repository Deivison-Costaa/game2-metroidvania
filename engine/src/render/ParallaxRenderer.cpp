#include "engine/render/ParallaxRenderer.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace eng::render {

void drawParallax(SpriteBatch& batch,
                  const Camera& worldCam,
                  const std::vector<ParallaxLayer>& layers)
{
    const glm::vec3 camPos = worldCam.position();

    for (const auto& layer : layers) {
        if (!layer.tex) continue;

        // Parallax X: the layer appears to scroll factorX times the camera.
        // Formula: layerWorldX = camPos.x * (1 - factorX)
        // When factorX=0: layer stays fixed (background appears stationary).
        // When factorX=1: layer moves with world (no parallax effect).
        const float layerX = camPos.x * (1.f - layer.factorX);
        const float layerY = camPos.y + layer.yOffsetFromCam;

        // Width of one texture repetition in world units
        const float quadW = static_cast<float>(layer.tex->width()) /
                            layer.pixelsPerMeter;

        // Tile enough copies to cover at least 3 screen-widths of scrolling
        batch.begin(worldCam);
        for (int rep = -2; rep <= 2; ++rep) {
            batch.draw(*layer.tex,
                       glm::vec3{layerX + static_cast<float>(rep) * quadW,
                                 layerY,
                                 layer.depthZ},
                       glm::vec2{quadW, layer.worldH});
        }
        batch.end();
    }
}

} // namespace eng::render
