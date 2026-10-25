<script lang="ts">
    import {onMount} from "svelte";
    import {fade} from "svelte/transition";
    import {prefersReducedMotion, Spring} from "svelte/motion";
    import {socketService} from "$lib/services/socket.svelte";
    import {ENDPOINTS} from "$lib/config";
    import {busyStore} from "$lib/services/busy.svelte";
    import {apiRequest, type ApiResponse} from "$lib/api";
    import type {SystemConfig} from "$lib/interfaces";
    import {FADE_MS, SPRING_DEFAULT} from "$lib/motion";
    import Icon, {type IconName} from "$lib/Icon.svelte";
    import Apps from "$lib/Apps.svelte";
    import Status from "$lib/Status.svelte";
    import Config from "$lib/Config.svelte";

    const TABS: { id: string, label: string, icon: IconName }[] = [
        { id: 'apps', label: 'Apps', icon: 'overview' },
        { id: 'status', label: 'Status', icon: 'status' },
        { id: 'config', label: 'Settings', icon: 'settings' },
    ];

    // Sidebar rows are h-9 with a 2px gap; the selection pill springs between them
    const ROW_PITCH = 38;

    let config = $state<ApiResponse<SystemConfig>>({ data: null, error: null, loading: false });
    let activeTab = $state('apps');
    let scrolled = $state(false);

    let activeIndex = $derived(TABS.findIndex(t => t.id === activeTab));
    // svelte-ignore state_referenced_locally
    const selection = new Spring(activeIndex, SPRING_DEFAULT);

    $effect(() => {
        selection.set(activeIndex, { instant: prefersReducedMotion.current });
    });

    onMount(() => {
        socketService.connect(ENDPOINTS.WEB_SOCKET);
        loadConfig();
    })

    function navigate(tab: string) {
        if (tab === activeTab) return;
        activeTab = tab;
        window.scrollTo({ top: 0 });
    }

    async function loadConfig() {
        await busyStore.run(async () => {
           config = await apiRequest<SystemConfig>(ENDPOINTS.GET_SYSTEM_CONFIG);
        });
    }
</script>

<svelte:window onscroll={() => scrolled = window.scrollY > 4}/>

{#snippet connection()}
    <span class="inline-flex items-center gap-1.5 text-[0.75rem] font-medium text-base-content/55">
        <span class="size-1.5 rounded-full transition-colors duration-300 {socketService.isConnected ? 'bg-success' : 'bg-base-content/30'}"></span>
        {socketService.isConnected ? 'Connected' : 'Offline'}
    </span>
{/snippet}

<div class="min-h-dvh lg:pl-64">

    <!-- Desktop sidebar: a heavier material separating navigation from content -->
    <aside class="material fixed inset-y-0 left-0 z-20 hidden w-64 flex-col border-r border-base-content/8 px-3 pt-6 pb-4 lg:flex">
        <div class="mb-6 px-3">
            <p class="text-[1.0625rem] font-semibold tracking-[-0.015em]">Matrix Controller</p>
            {@render connection()}
        </div>

        <nav aria-label="Main" class="relative">
            <div aria-hidden="true" class="absolute inset-x-0 top-0 h-9 rounded-[0.5rem] bg-base-content/8 will-change-transform"
                 style="transform: translateY({selection.current * ROW_PITCH}px)"></div>
            <ul class="relative grid gap-0.5">
                {#each TABS as tab}
                    <li>
                        <button type="button" aria-current={activeTab === tab.id ? 'page' : undefined}
                                class="pressable flex h-9 w-full items-center gap-2.5 rounded-[0.5rem] px-3 text-[0.9375rem] transition-colors {activeTab === tab.id ? 'font-semibold' : 'text-base-content/80 hover:bg-base-content/4'}"
                                onclick={() => navigate(tab.id)}>
                            <Icon name={tab.icon} class="size-5 {activeTab === tab.id ? 'text-primary' : 'text-base-content/60'}" strokeWidth={1.75}/>
                            {tab.label}
                        </button>
                    </li>
                {/each}
            </ul>
        </nav>
    </aside>

    <!-- Mobile top bar: translucent, with a hairline only once content scrolls beneath it -->
    <header class="material sticky top-0 z-20 flex h-12 items-center justify-between border-b px-4 pt-[env(safe-area-inset-top)] transition-colors duration-200 lg:hidden {scrolled ? 'border-base-content/10' : 'border-transparent'}">
        <span class="text-[0.9375rem] font-semibold tracking-[-0.01em]">Matrix Controller</span>
        {@render connection()}
    </header>

    <main class="mx-auto max-w-3xl px-4 pt-6 pb-[calc(6rem+env(safe-area-inset-bottom))] sm:px-6 lg:px-10 lg:pt-14 lg:pb-14">
        {#key activeTab}
            <div in:fade={{ duration: prefersReducedMotion.current ? 0 : FADE_MS }}>
                {#if activeTab === 'apps'}
                    <Apps config={config.data}/>
                {:else if activeTab === 'status'}
                    <Status/>
                {:else if activeTab === 'config'}
                    <Config config={config.data} reload={loadConfig}/>
                {/if}
            </div>
        {/key}
    </main>

    <!-- Mobile tab bar -->
    <nav aria-label="Main" class="material fixed inset-x-0 bottom-0 z-20 border-t border-base-content/10 pb-[env(safe-area-inset-bottom)] lg:hidden">
        <ul class="mx-auto grid max-w-md grid-cols-3">
            {#each TABS as tab}
                <li>
                    <button type="button" aria-current={activeTab === tab.id ? 'page' : undefined}
                            class="pressable flex h-14 w-full flex-col items-center justify-center gap-0.5 text-[0.6875rem] font-medium tracking-[0.01em] transition-colors {activeTab === tab.id ? 'text-primary' : 'text-base-content/45'}"
                            onclick={() => navigate(tab.id)}>
                        <Icon name={tab.icon} class="size-6" strokeWidth={activeTab === tab.id ? 2 : 1.5}/>
                        {tab.label}
                    </button>
                </li>
            {/each}
        </ul>
    </nav>

</div>
