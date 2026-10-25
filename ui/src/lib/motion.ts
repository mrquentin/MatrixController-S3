// Svelte springs integrate per 60fps frame with unit mass, so
//   stiffness = (2π / (response · 60))²   and   damping = 2 · ζ · √stiffness
// These map to Apple's default UI spring: damping ratio ζ = 1.0, response ≈ 0.32s.
export const SPRING_DEFAULT = { stiffness: 0.107, damping: 0.654 };

// Short cross-fade used for content changes; collapses to nothing with reduced motion
export const FADE_MS = 180;
