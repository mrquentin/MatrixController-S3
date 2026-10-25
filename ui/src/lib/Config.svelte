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
    import ConfirmDialog from "$lib/ConfirmDialog.svelte";
    import WifiPage from "$lib/WifiPage.svelte";
    import OptionsPage from "$lib/OptionsPage.svelte";
    import {onMount} from "svelte";
    import {fade, fly} from "svelte/transition";
    import {prefersReducedMotion} from "svelte/motion";
    import {FADE_MS} from "$lib/motion";

    let { config, reload }: { config: SystemConfig | null, reload: () => Promise<void> } = $props();

    // WiFi and restart: each change restarts the controller, so each is confirmed first
    let wifiSsid = $state<string | null>(null);
    // Pushed page shown instead of the settings list
    let subpage = $state<'wifi' | 'timezone' | null>(null);
    let confirmForget = $state(false);
    let confirmRestart = $state(false);
    let restarting = $state<{ title: string, message: string } | null>(null);
    let actionError = $state<string | null>(null);

    async function loadWifi() {
        const response = await apiRequest<{ ssid: string }>(ENDPOINTS.GET_WIFI);
        if (response.data) wifiSsid = response.data.ssid;
    }

    onMount(() => {
        loadWifi();
    });

    // Same pushed-view motion as the app pages
    function push(node: Element, { direction }: { direction: 1 | -1 }) {
        return prefersReducedMotion.current
            ? fade(node, { duration: FADE_MS })
            : fly(node, { x: 24 * direction, duration: 260, opacity: 0 });
    }

    function show(page: typeof subpage) {
        subpage = page;
        window.scrollTo({ top: 0 });
    }

    // Asks the controller for something that ends in a restart. When it is expected back on this
    // address, waits for it and picks its settings up again
    async function restartWith(url: string, body: object | undefined,
                               notice: { title: string, message: string }, comesBack: boolean) {
        actionError = null;
        const response = await busyStore.run(() => apiRequest<null>(url, "POST", body));
        if (response.error) {
            actionError = response.error;
            return;
        }
        restarting = notice;
        if (comesBack) waitForRestart();
    }

    async function waitForRestart() {
        // Give it time to actually go down before asking
        await new Promise(resolve => setTimeout(resolve, 5000));
        for (let attempt = 0; attempt < 30 && restarting; attempt++) {
            try {
                const response = await fetch(ENDPOINTS.GET_SYSTEM_STATUS, { signal: AbortSignal.timeout(2000) });
                if (response.ok) {
                    restarting = null;
                    await Promise.all([reload(), loadWifi()]);
                    return;
                }
            } catch {
                // Still restarting
            }
            await new Promise(resolve => setTimeout(resolve, 2000));
        }
    }

    function joinWifi(ssid: string, password: string) {
        // Back to Settings, where the restart notice shows
        show(null);
        restartWith(ENDPOINTS.POST_WIFI, { ssid, password }, {
            title: `Joining ${ssid}`,
            message: 'The controller is restarting. If it ends up on another network or address, reopen this page at http://matrixcontroller.local. If it can\'t join, the panel shows its setup network instead.'
        }, true);
    }

    function forgetWifi() {
        restartWith(ENDPOINTS.POST_WIFI_FORGET, undefined, {
            title: 'WiFi forgotten',
            message: 'The controller is restarting in setup mode. Join the network shown on the panel to set up WiFi again.'
        }, false);
    }

    function restart() {
        restartWith(ENDPOINTS.POST_RESTART, undefined, {
            title: 'Restarting',
            message: 'This page picks the controller up again once it is back.'
        }, true);
    }

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
    // The row has little room: "Paris, Berlin, Rome (CET/CEST)" shows as "Paris, Berlin, Rome"
    let timezoneLabel = $derived((timezoneOptions.find(t => t.value === modifiedConfig.timezone)?.label ?? modifiedConfig.timezone)
        .replace(/\s*\([^)]*\)$/, ''));

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

{#if subpage === 'wifi'}
    <div in:push={{ direction: 1 }}>
        <WifiPage current={wifiSsid ?? ''} onBack={() => show(null)} onJoin={joinWifi}/>
    </div>
{:else if subpage === 'timezone'}
    <div in:push={{ direction: 1 }}>
        <OptionsPage title="Time zone" backLabel="Settings" options={timezoneOptions}
                     bind:value={modifiedConfig.timezone} onBack={() => show(null)}
                     footer="Applies once you save the settings."/>
    </div>
{:else}
    <div in:push={{ direction: -1 }}>
        <PageHeader title="Settings"/>

        {#if restarting}
            <div role="status" class="mb-6 flex items-start gap-3 rounded-box bg-primary/10 px-4 py-3 text-primary">
                <Icon name="power" class="mt-0.5 size-5 shrink-0"/>
                <div>
                    <p class="font-semibold">{restarting.title}</p>
                    <p class="text-[0.875rem] text-base-content/70">{restarting.message}</p>
                </div>
            </div>
        {/if}

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
                    <button type="button" class="grouped-row pressable-row" onclick={() => show('timezone')}>
                        <span class="shrink-0">Time zone</span>
                        <span class="row-value">{timezoneLabel}</span>
                        <Icon name="chevron" class="size-4 shrink-0 text-base-content/30" strokeWidth={2.5}/>
                    </button>
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

            <section>
                <h2 class="section-header">WiFi</h2>
                <div class="grouped">
                    <div class="grouped-row">
                        <span>Network</span>
                        <span class="row-value">{wifiSsid ?? '—'}</span>
                    </div>
                    <button type="button" class="grouped-row pressable-row text-primary disabled:opacity-40"
                            disabled={!!restarting} onclick={() => show('wifi')}>
                        <span>Change network</span>
                        <Icon name="chevron" class="ml-auto size-4 text-base-content/30" strokeWidth={2.5}/>
                    </button>
                    <button type="button" class="grouped-row pressable-row text-error disabled:opacity-40"
                            disabled={!!restarting} onclick={() => confirmForget = true}>
                        Forget network
                    </button>
                </div>
                <p class="section-footer">Forgetting the network restarts the controller in setup mode, with its own WiFi network shown on the panel.</p>
            </section>

            <section>
                <h2 class="section-header">Device</h2>
                <div class="grouped">
                    <button type="button" class="grouped-row pressable-row text-primary disabled:opacity-40"
                            disabled={!!restarting} onclick={() => confirmRestart = true}>
                        Restart controller
                    </button>
                </div>
                {#if actionError}
                    <p class="section-footer text-error!">The controller didn't respond: {actionError}</p>
                {/if}
            </section>

        </div>
    </div>
{/if}

<ConfirmDialog bind:open={confirmForget} title="Forget {wifiSsid || 'this network'}?" destructive
               message="The controller restarts in setup mode and leaves this network. You'll need to join its setup network to connect it again."
               confirmLabel="Forget" onConfirm={forgetWifi}/>

<ConfirmDialog bind:open={confirmRestart} title="Restart the controller?"
               message="It takes a few seconds, and the panel shows its start-up screens again."
               confirmLabel="Restart" onConfirm={restart}/>
