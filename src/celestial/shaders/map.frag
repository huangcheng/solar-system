#version 330 core
// Flat "unwound globe" map: samples the equirectangular day/night textures by
// UV and computes day/night from the TRUE ECEF sun direction (no spin) — the
// faithful real-world terminator. No brown dusk tint (clean transition).
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uDay;
uniform sampler2D uNight;
uniform vec3  uSunDir;            // ECEF unit sun (real wall-clock sun)
uniform float uHasDay;
uniform float uHasNight;
uniform float uUseNightTexture;   // 1 = city-lights night texture, 0 = dimmed day
uniform float uShowGrid;
uniform float uHasHome;
uniform vec3  uHomeDir;           // ECEF unit vector of the home location
uniform float uTime;

const float PI  = 3.14159265358979;
const float HPI = 1.57079632679490;

const float kGridStepRad = PI / 12.0;      // 15 degrees
const float kGridLinePx  = 1.2;
const vec3  kGridColor   = vec3(1.0, 0.70, 0.28);   // warm amber (matches the globe grid)
const float kGridAlpha   = 0.22;

float gridLine(float coord) {
    float d = abs(fract(coord / kGridStepRad + 0.5) - 0.5) * kGridStepRad;
    float w = fwidth(coord) * kGridLinePx;
    return 1.0 - smoothstep(0.0, w, d);
}

void main() {
    vec2 uv = vUv;
    float lon = (uv.x * 2.0 - 1.0) * PI;    // [-PI, PI]
    float lat = (uv.y * 2.0 - 1.0) * HPI;   // [-PI/2, PI/2]

    float cl = cos(lat);
    vec3 p   = vec3(cl * cos(lon), cl * sin(lon), sin(lat));  // ECEF surface normal
    vec3 sun = normalize(uSunDir);

    float cosGeo   = dot(p, sun);
    float dayFactor = smoothstep(-0.10, 0.20, cosGeo);   // same band as earth.frag / TerminatorModel

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5 && uUseNightTexture > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);
    // Match earth.frag: gamma + gain lifts dim city lights so the night side
    // reads densely populated; faint earthshine keeps just enough continent
    // outline without washing the lights out.
    vec3 nightLit = pow(night, vec3(0.8)) * 1.35;
    vec3 darkSide = nightLit + day * 0.14 + vec3(0.012, 0.014, 0.022);
    vec3 color = mix(darkSide, day, dayFactor);
    // NOTE: no brown dusk tint here (deliberately clean day/night transition).

    // Grid: straight lon/lat lines every 15 degrees.
    if (uShowGrid > 0.5) {
        float grid = max(gridLine(lat), gridLine(lon));
        color = mix(color, kGridColor, grid * kGridAlpha);
    }

    // Home marker: angular distance on the sphere to the home point (glued to
    // the surface, same look as the globe beacon).
    if (uHasHome > 0.5) {
        float dd = acos(clamp(dot(p, normalize(uHomeDir)), -1.0, 1.0));
        float pulse = 0.5 + 0.5 * sin(uTime * 2.5);
        vec3  warmOuter = vec3(1.0, 0.50, 0.10);
        vec3  warmInner = vec3(1.0, 0.82, 0.30);
        float glowR = 0.045 + 0.015 * pulse;
        float glow  = exp(-3.0 * (dd / glowR) * (dd / glowR));
        color += warmOuter * glow * (0.40 + 0.30 * pulse);
        float ring = exp(-((dd - 0.022) / 0.004) * ((dd - 0.022) / 0.004)) * 0.55;
        color += warmInner * ring;
        float core = smoothstep(0.010, 0.004, dd);
        color = mix(color, vec3(1.0), core);
    }

    FragColor = vec4(color, 1.0);
}
