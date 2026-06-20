#version 330 core
// Ring annulus vertex stage (Saturn). The ring is a flat mesh lying in the XY
// plane (the equatorial plane of this codebase's Z-up spheres). Geometry supplies:
//   location 0: vec3 aPos  -- world/object position of the ring vertex
//   location 1: vec2 aUv   -- (radial u, angular v) into the ring texture
// It is transformed by a single uMVP (no lighting, no model decomposition is
// needed because the ring is flat and unshaded).
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
uniform mat4 uMVP;
out vec2 vUv;

void main() {
    vUv = aUv;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
