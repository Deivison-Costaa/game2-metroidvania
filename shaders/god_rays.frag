#version 450 core

// God rays via screen-space radial blur (Crytek / Shawn Hargreaves technique).
// Reads the bright-pass texture and scatters light outward from the sun UV.

in vec2 vUV;

uniform sampler2D uBrightPass;
uniform vec2      uSunUV;    // sun screen position in [0,1] UV space
uniform float     uDensity;  // step scale (0.96 default)
uniform float     uWeight;   // per-sample weight (0.1)
uniform float     uDecay;    // illumination decay per step (0.95)
uniform float     uExposure; // final multiplier (0.25)
uniform vec3      uColor;    // tint color for the rays

out vec4 fragColor;

void main() {
    // Only emit when sun is visible in the frustum
    float inFrustum = float(uSunUV.x >= 0.0 && uSunUV.x <= 1.0 &&
                            uSunUV.y >= 0.0 && uSunUV.y <= 1.0);

    const int NUM_SAMPLES = 64;
    vec2 delta   = (vUV - uSunUV) / float(NUM_SAMPLES) * uDensity;
    vec2 uv      = vUV;
    float illum  = 1.0;
    vec3  accum  = vec3(0.0);

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        uv   -= delta;
        vec3 s = texture(uBrightPass, clamp(uv, 0.0, 1.0)).rgb;
        accum += s * illum * uWeight;
        illum *= uDecay;
    }

    fragColor = vec4(accum * uExposure * uColor * inFrustum, 1.0);
}
