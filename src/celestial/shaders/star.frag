#version 330 core
// Emissive star (Sun) surface. Unlike Earth there is no day/night terminator,
// no terrain relief and no specular glint — the photosphere emits everywhere at
// (near) full intensity. Two small touches keep it from looking like a flat
// sticker:
//   * a slow, subtle pulse (±5%) so the disc feels alive, and
//   * gentle limb darkening (real photospheres dim toward the edge), which
//     reads as a sphere instead of a cut-out circle.
//
// Varying names mirror earth.vert exactly (vLocal / vWorld / vUV) so the Sun
// shares the same sphere VAO. uMVP/uModel are unused here but declared in the
// vertex stage; uViewPos matches earth.frag's convention for the limb term.
in vec3 vLocal;   // object-space unit-sphere position (== object normal)
in vec3 vWorld;   // world-space position
in vec2 vUV;      // sphere UV
out vec4 FragColor;

uniform sampler2D uDay;     // optional star surface texture
uniform float uHasDay;      // 1 when uDay is bound
uniform vec3  uViewPos;     // camera position, world space (for the limb term)
uniform float uTime;        // seconds, drives the pulse

void main() {
    // Full-intensity albedo. Fall back to a warm solar yellow when no texture.
    vec3 base = (uHasDay > 0.5) ? texture(uDay, vUV).rgb
                                : vec3(1.0, 0.85, 0.4);

    // Slow, subtle pulse so the Sun feels alive (±5%, ~12.6s period).
    float pulse = 0.95 + 0.05 * sin(uTime * 0.5);

    // Gentle limb darkening using the geometric world normal (same convention
    // as earth.frag, which treats the unit sphere's world position as its
    // normal). mu = 1 at the disc center, -> 0 at the limb.
    vec3 nGeo    = normalize(vWorld);
    vec3 viewDir = normalize(uViewPos - vWorld);
    float mu     = max(dot(nGeo, viewDir), 0.0);
    float limb   = 0.80 + 0.20 * mu;   // 1.0 center -> 0.80 limb

    FragColor = vec4(base * pulse * limb, 1.0);
}
