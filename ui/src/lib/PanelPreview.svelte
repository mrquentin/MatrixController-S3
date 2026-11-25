<script lang="ts">
    import type {ConfigState} from "$lib/interfaces";

    let { config, label, title }: { config: ConfigState, label: string, title: string } = $props();

    let width = $derived(config.limits.panelWidth);
    let height = $derived(config.limits.panelHeight);
    let brightness = $derived(Math.round(config.config.display.brightness / config.limits.maxBrightness * 100));
</script>

<!-- One dot per LED, lit at the saved brightness -->
<figure class="overflow-hidden rounded-box bg-black p-4 shadow-[0_8px_30px_rgb(0_0_0/0.12)] sm:p-6">
    <svg viewBox="0 0 {width} {height}" class="block w-full" role="img"
         aria-label="{width} by {height} LED panel at {brightness}% brightness">
        <defs>
            <pattern id="led" width="1" height="1" patternUnits="userSpaceOnUse">
                <circle cx="0.5" cy="0.5" r="0.36" fill="white"/>
            </pattern>
            <mask id="led-mask">
                <rect width="100%" height="100%" fill="url(#led)"/>
            </mask>
            <linearGradient id="led-glow" x1="0" y1="0" x2="1" y2="1">
                <stop offset="0" stop-color="#ff375f"/>
                <stop offset="0.3" stop-color="#ff9f0a"/>
                <stop offset="0.55" stop-color="#30d158"/>
                <stop offset="0.8" stop-color="#0a84ff"/>
                <stop offset="1" stop-color="#bf5af2"/>
            </linearGradient>
        </defs>
        <rect width="100%" height="100%" fill="#1c1c1e" mask="url(#led-mask)"/>
        <rect width="100%" height="100%" fill="url(#led-glow)" mask="url(#led-mask)"
              style="opacity: {0.15 + 0.85 * brightness / 100}; transition: opacity 300ms ease"/>
    </svg>
    <figcaption class="mt-4 flex items-end justify-between gap-4 text-white">
        <div class="min-w-0">
            <p class="text-[0.75rem] font-medium uppercase tracking-[0.04em] text-white/50">{label}</p>
            <p class="truncate text-[1.375rem] font-semibold tracking-[-0.02em]">{title}</p>
        </div>
        <p class="shrink-0 pb-1 text-right text-[0.8125rem] text-white/60 tabular-nums">
            {width} × {height} · {brightness}%
        </p>
    </figcaption>
</figure>
