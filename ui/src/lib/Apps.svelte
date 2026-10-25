<script lang="ts">
    import type {AppConfig, AppsState, SystemConfig} from "$lib/interfaces";
    import {apiRequest, type ApiResponse} from "$lib/api";
    import {busyStore} from "$lib/services/busy.svelte";
    import {DEFAULT_DISPLAY_CONFIG, ENDPOINTS} from "$lib/config";
    import {onMount} from "svelte";
    import {fade, fly} from "svelte/transition";
    import {prefersReducedMotion} from "svelte/motion";
    import {FADE_MS} from "$lib/motion";
    import PageHeader from "$lib/PageHeader.svelte";
    import PanelPreview from "$lib/PanelPreview.svelte";
    import AppDetail from "$lib/AppDetail.svelte";
    import Icon, {type IconName} from "$lib/Icon.svelte";

    let { config }: { config: SystemConfig | null } = $props();

    // Known apps get their own glyph and tint; new firmware apps fall back to a generic tile
    const APP_STYLE: Record<string, { icon: IconName, tint: string }> = {
        clock: { icon: 'clock', tint: 'bg-[#ff9500]' },
        text: { icon: 'text', tint: 'bg-[#007aff]' },
        weather: { icon: 'cloud', tint: 'bg-[#32ade6]' },
        f1: { icon: 'flag', tint: 'bg-[#ff3b30]' },
    };
    const FALLBACK_STYLE = { icon: 'overview' as IconName, tint: 'bg-[#8e8e93]' };

    let apps = $state<ApiResponse<AppsState>>({data: null, error: null, loading: false});
    let openId = $state<string | null>(null);

    let display = $derived({ ...DEFAULT_DISPLAY_CONFIG, ...config?.display });
    let activeApp = $derived(apps.data?.apps.find(a => a.id === apps.data?.active));
    let openApp = $derived(apps.data?.apps.find(a => a.id === openId));

    // Pushed views slide in from the trailing edge; going back brings the list in from the leading edge
    function push(node: Element, { direction }: { direction: 1 | -1 }) {
        return prefersReducedMotion.current
            ? fade(node, { duration: FADE_MS })
            : fly(node, { x: 24 * direction, duration: 260, opacity: 0 });
    }

    async function load() {
        await busyStore.run(async () => {
            apps = await apiRequest<AppsState>(ENDPOINTS.GET_APPS);
        });
    }

    // Optimistic: the UI switches at once and rolls back if the board refuses
    async function activate(id: string) {
        if (!apps.data) return;
        const previous = apps.data.active;
        apps.data.active = id;
        apps.error = null;
        const res = await busyStore.run(() => apiRequest<null>(ENDPOINTS.POST_APP_ACTIVE, "POST", {id}));
        if (res.error && apps.data) {
            apps.data.active = previous;
            apps.error = res.error;
        }
    }

    function open(id: string | null) {
        openId = id;
        window.scrollTo({ top: 0 });
    }

    function saved(id: string, appConfig: AppConfig) {
        const app = apps.data?.apps.find(a => a.id === id);
        if (app) app.config = appConfig;
    }

    onMount(load);
</script>

{#if openApp}
    <div in:push={{ direction: 1 }}>
        {#key openApp.id}
            <AppDetail app={openApp} active={apps.data?.active === openApp.id}
                       onActivate={() => activate(openApp.id)} onBack={() => open(null)}
                       onSaved={c => saved(openApp.id, c)}/>
        {/key}
    </div>
{:else}
    <div in:push={{ direction: -1 }}>
        <PageHeader title="Apps"/>

        {#if apps.error}
            <div role="alert" class="mb-6 flex items-start gap-3 rounded-box bg-error/10 px-4 py-3 text-error">
                <Icon name="warning" class="mt-0.5 size-5 shrink-0"/>
                <div>
                    <p class="font-semibold">Couldn't reach the controller</p>
                    <p class="text-[0.875rem] opacity-80">{apps.error}</p>
                </div>
            </div>
        {/if}

        <div class="mb-8">
            <PanelPreview {display} label="Now showing" title={activeApp?.name ?? 'Nothing'}/>
        </div>

        {#if apps.data}
            <section>
                <h2 class="section-header">Library</h2>
                <ul class="grouped">
                    {#each apps.data.apps as app (app.id)}
                        {@const style = APP_STYLE[app.id] ?? FALLBACK_STYLE}
                        {@const isActive = apps.data.active === app.id}
                        <li class="grouped-row gap-0 p-0">
                            <button type="button" class="pressable-row flex min-w-0 flex-1 items-center gap-3 py-2.5 pl-4 text-left transition-colors"
                                    onclick={() => open(app.id)}>
                                <span class="flex size-8 shrink-0 items-center justify-center rounded-[0.5rem] text-white {style.tint}">
                                    <Icon name={style.icon} class="size-5" strokeWidth={2}/>
                                </span>
                                <span class="min-w-0">
                                    <span class="block truncate">{app.name}</span>
                                    <span class="block text-[0.8125rem] {isActive ? 'text-success' : 'text-base-content/50'}">
                                        {isActive ? 'On the panel' : app.schema.length === 0 ? 'No settings' : `${app.schema.length} settings`}
                                    </span>
                                </span>
                                <Icon name="chevron" class="ml-auto size-4 shrink-0 text-base-content/30" strokeWidth={2.5}/>
                            </button>
                            <span class="flex w-24 shrink-0 justify-end pr-4">
                                {#if isActive}
                                    <span class="flex h-7 items-center gap-1 text-[0.8125rem] font-semibold text-success">
                                        <Icon name="check" class="size-4" strokeWidth={2.5}/> Showing
                                    </span>
                                {:else}
                                    <button type="button" class="pressable h-7 rounded-full bg-primary/12 px-3.5 text-[0.8125rem] font-semibold text-primary"
                                            onclick={() => activate(app.id)} aria-label="Show {app.name} on the panel">
                                        Show
                                    </button>
                                {/if}
                            </span>
                        </li>
                    {/each}
                </ul>
                <p class="section-footer">Only one app is on the panel at a time. Tap an app to change its settings.</p>
            </section>
        {/if}
    </div>
{/if}
