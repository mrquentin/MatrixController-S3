<script lang="ts">
    import type {Snippet} from "svelte";
    import {fade} from "svelte/transition";
    import {prefersReducedMotion} from "svelte/motion";
    import {busyStore} from "$lib/services/busy.svelte";
    import {FADE_MS} from "$lib/motion";

    let { title, subtitle, actions }: { title: string, subtitle?: string, actions?: Snippet } = $props();
</script>

<header class="mb-7 flex items-end gap-3">
    <div class="min-w-0">
        <h1 class="large-title truncate">{title}</h1>
        {#if subtitle}
            <p class="mt-1 text-[0.9375rem] text-base-content/55">{subtitle}</p>
        {/if}
    </div>
    <!-- Ongoing status: work in flight never blocks the page -->
    {#if busyStore.active}
        <span class="loading loading-spinner loading-sm mb-1.5 text-base-content/40" role="status" aria-label="Loading"
              transition:fade={{ duration: prefersReducedMotion.current ? 0 : FADE_MS }}></span>
    {/if}
    {#if actions}
        <div class="ml-auto flex shrink-0 items-center gap-2">{@render actions()}</div>
    {/if}
</header>
