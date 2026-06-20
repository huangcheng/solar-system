#version 330 core
// Ring annulus fragment stage (Saturn). A straight texture lookup with alpha —
// saturn_ring.png has transparency for the Cassini division and gaps.
// (Blending + depth-write-off are handled in the CelestialBody C++ draw call.)
// No lighting: the ring is dust/ice lit flat.
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D uRing;

void main() {
    FragColor = texture(uRing, vUv);
}
