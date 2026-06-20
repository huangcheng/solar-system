#version 330 core
// Ring annulus fragment stage (Saturn). A straight texture lookup with alpha —
// saturn_ring.png has transparency for the Cassini division and gaps, so the
// C++ draw call (later task) enables GL_BLEND and a depth-write-off pass to
// composite it correctly. No lighting: the ring is dust/ice lit flat.
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D uRing;

void main() {
    FragColor = texture(uRing, vUv);
}
