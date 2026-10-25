<script lang="ts">
    import {apiRequest} from "$lib/api";
    import type {SystemConfig} from "$lib/interfaces";
    import {busyStore} from "$lib/services/busy.svelte";
    import {themeStore, type ThemeMode} from "$lib/services/theme.svelte";
    import {DEFAULT_DISPLAY_CONFIG, DEFAULT_TIMEZONE, DISPLAY_HEIGHTS, ENDPOINTS, MAX_BRIGHTNESS, TIMEZONES} from "$lib/config";
    import PageHeader from "$lib/PageHeader.svelte";
    import SegmentedControl from "$lib/SegmentedControl.svelte";
    import SaveBar from "$lib/SaveBar.svelte";
    import Icon from "$lib/Icon.svelte";

    let { config, reload }: { config: SystemConfig | null, reload: () => Promise<void> } = $props();

    function normalize(source: SystemConfig | null): SystemConfig {
        return {
            debug: source?.debug ?? false,
            // Older config.json files have no display section
            display: { ...DEFAULT_DISPLAY_CONFIG, ...source?.display },
            timezone: source?.timezone ?? DEFAULT_TIMEZONE
        };
    }

    let saved = $derived(normalize(config));
    // svelte-ignore state_referenced_locally
    let modifiedConfig = $state<SystemConfig>(normalize(config));

    // Pick up the device's values whenever they (re)load, e.g. if this page opened before the first fetch finished
    $effect(() => {
        if (config) modifiedConfig = normalize(config);
    });

    let saveError = $state<string | null>(null);

    let widthValid = $derived(Number.isInteger(modifiedConfig.display.width)
        && modifiedConfig.display.width >= 16 && modifiedConfig.display.width <= 256);
    let dirty = $derived(JSON.stringify(modifiedConfig) !== JSON.stringify(saved));
    let brightnessPercent = $derived(Math.round(modifiedConfig.display.brightness / MAX_BRIGHTNESS * 100));

    const heightOptions = DISPLAY_HEIGHTS.map(h => ({ value: h, label: `${h}` }));
    const themeOptions: { value: ThemeMode, label: string }[] = [
        { value: 'system', label: 'Automatic' },
        { value: 'light', label: 'Light' },
        { value: 'dark', label: 'Dark' },
    ];

    let theme = $state(themeStore.mode);
    $effect(() => themeStore.set(theme));

    // Keep a timezone set outside the UI selectable instead of silently replacing it
    let timezoneOptions = $derived(TIMEZONES.some(t => t.value === modifiedConfig.timezone)
        ? TIMEZONES : [...TIMEZONES, { label: modifiedConfig.timezone, value: modifiedConfig.timezone }]);

    async function save(): Promise<boolean> {
        saveError = null;
        return busyStore.run(async () => {
            const response = await apiRequest<null>(ENDPOINTS.POST_SYSTEM_CONFIG, "POST", { ...modifiedConfig });
            if (response.error) {
                saveError = response.error;
                return false;
            }
            await reload();
            return true;
        });
    }

    function revert() {
        modifiedConfig = normalize(config);
        saveError = null;
    }
</script>

<PageHeader title="Settings"/>

<div class="grid gap-8">

    <section>
        <h2 class="section-header">Panel</h2>
        <div class="grouped">
            <label class="grouped-row cursor-text">
                <span>Width</span>
                <span class="ml-auto flex items-center gap-1 text-base-content/55">
                    <input type="number" inputmode="numeric" min="16" max="256" step="1"
                           bind:value={modifiedConfig.display.width}
                           aria-invalid={!widthValid}
                           class="w-12 bg-transparent text-right tabular-nums outline-none focus:text-base-content {widthValid ? '' : 'text-error'}"/>
                    px
                </span>
            </label>
            <div class="grouped-row">
                <span class="shrink-0">Height</span>
                <div class="ml-auto w-44">
                    <SegmentedControl label="Height in pixels" options={heightOptions} bind:value={modifiedConfig.display.height}/>
                </div>
            </div>
            <div class="grouped-row flex-col items-stretch gap-2 py-3">
                <div class="flex items-center">
                    <span>Brightness</span>
                    <span class="row-value">{brightnessPercent}%</span>
                </div>
                <input type="range" min="0" max={MAX_BRIGHTNESS} aria-label="Brightness"
                       bind:value={modifiedConfig.display.brightness} class="slider"
                       style="--fill: {modifiedConfig.display.brightness / MAX_BRIGHTNESS * 100}%"/>
            </div>
            <label class="grouped-row cursor-pointer">
                <span>Double buffering</span>
                <input type="checkbox" bind:checked={modifiedConfig.display.doubleBuffer} class="switch ml-auto"/>
            </label>
        </div>
        {#if !widthValid}
            <p class="section-footer text-error!">Width must be a whole number between 16 and 256.</p>
        {:else}
            <p class="section-footer">Brightness applies when you save. Resolution and double buffering take effect after the controller restarts.</p>
        {/if}
    </section>

    <section>
        <h2 class="section-header">Time</h2>
        <div class="grouped">
            <label class="grouped-row cursor-pointer">
                <span class="shrink-0">Time zone</span>
                <span class="relative ml-auto flex min-w-0 items-center gap-1 text-base-content/55">
                    <select bind:value={modifiedConfig.timezone}
                            class="min-w-0 cursor-pointer appearance-none truncate bg-transparent text-right outline-none focus-visible:text-primary">
                        {#each timezoneOptions as timezone (timezone.value)}
                            <option value={timezone.value}>{timezone.label}</option>
                        {/each}
                    </select>
                    <Icon name="chevron" class="size-3.5 shrink-0 rotate-90 opacity-60" strokeWidth={2.5}/>
                </span>
            </label>
        </div>
        <p class="section-footer">Used by the Clock app. Daylight saving time is handled automatically.</p>
    </section>

    <section>
        <h2 class="section-header">Developer</h2>
        <div class="grouped">
            <label class="grouped-row cursor-pointer">
                <span>Debug logging</span>
                <input type="checkbox" bind:checked={modifiedConfig.debug} class="switch ml-auto"/>
            </label>
        </div>
    </section>

    <SaveBar {dirty} valid={widthValid} error={saveError} errorTitle="Settings weren't saved" onSave={save} onRevert={revert}/>

    <section>
        <h2 class="section-header">Appearance</h2>
        <div class="grouped">
            <div class="grouped-row">
                <SegmentedControl label="Appearance" options={themeOptions} bind:value={theme}/>
            </div>
        </div>
        <p class="section-footer">Stored in this browser only.</p>
    </section>

</div>
