<script lang="ts">
    import {fade} from "svelte/transition";
    import {prefersReducedMotion} from "svelte/motion";
    import {busyStore} from "$lib/services/busy.svelte";
    import {FADE_MS} from "$lib/motion";
    import Icon from "$lib/Icon.svelte";

    // Save is only enabled when there is something valid to save; Revert appears with unsaved edits
    let { dirty, valid = true, error, errorTitle, onSave, onRevert }: {
        dirty: boolean,
        valid?: boolean,
        error: string | null,
        errorTitle: string,
        onSave: () => Promise<boolean>,
        onRevert: () => void
    } = $props();

    let justSaved = $state(false);
    let savedTimer: ReturnType<typeof setTimeout> | undefined;
    let fadeMs = $derived(prefersReducedMotion.current ? 0 : FADE_MS);

    async function save() {
        if (!await onSave()) return;
        justSaved = true;
        clearTimeout(savedTimer);
        savedTimer = setTimeout(() => justSaved = false, 2000);
    }
</script>

<div class="grid gap-2">
    {#if error}
        <div role="alert" class="flex items-start gap-3 rounded-box bg-error/10 px-4 py-3 text-error" transition:fade={{ duration: fadeMs }}>
            <Icon name="warning" class="mt-0.5 size-5 shrink-0"/>
            <div>
                <p class="font-semibold">{errorTitle}</p>
                <p class="text-[0.875rem] opacity-80">{error}</p>
            </div>
        </div>
    {/if}
    <button class="btn btn-primary pressable h-12 rounded-box text-[1.0625rem] font-semibold shadow-none {justSaved && !dirty ? 'disabled:bg-success disabled:text-success-content' : 'disabled:bg-base-content/10 disabled:text-base-content/35'}"
            onclick={save} disabled={!dirty || !valid || busyStore.active}>
        {#if justSaved && !dirty}
            <span class="flex items-center gap-1.5" in:fade={{ duration: fadeMs }}>
                <Icon name="check" class="size-5" strokeWidth={2.5}/> Saved
            </span>
        {:else}
            Save changes
        {/if}
    </button>
    <button class="btn btn-ghost pressable h-10 rounded-box font-normal text-primary transition-opacity duration-200 {dirty ? 'opacity-100' : 'pointer-events-none opacity-0'}"
            onclick={onRevert} tabindex={dirty ? 0 : -1} aria-hidden={!dirty}>
        Revert
    </button>
</div>
