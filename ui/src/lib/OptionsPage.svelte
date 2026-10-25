<script lang="ts" generics="T">
    import PageHeader from "$lib/PageHeader.svelte";
    import Icon from "$lib/Icon.svelte";

    // Pushed pick list, iOS Settings style: replaces a <select>, whose popup can't be styled.
    // Picking an option goes straight back
    let { title, backLabel, options, value = $bindable(), footer, onBack }: {
        title: string,
        backLabel: string,
        options: { value: T, label: string }[],
        value: T,
        footer?: string,
        onBack: () => void
    } = $props();

    // "Paris, Berlin, Rome (CET/CEST)" shows the bracketed part as detail on the trailing edge
    function split(label: string): { name: string, detail: string } {
        const match = /^(.*?)\s*\(([^)]*)\)$/.exec(label);
        return match ? { name: match[1], detail: match[2] } : { name: label, detail: '' };
    }

    function pick(option: T) {
        value = option;
        onBack();
    }
</script>

<button type="button" class="pressable -ml-1.5 mb-3 flex items-center gap-0.5 rounded-md py-1 pr-2 text-[1.0625rem] text-primary" onclick={onBack}>
    <Icon name="back" class="size-5" strokeWidth={2.5}/> {backLabel}
</button>

<PageHeader {title}/>

<section>
    <div class="grouped" role="listbox" aria-label={title}>
        {#each options as option (option.value)}
            {@const parts = split(option.label)}
            {@const selected = option.value === value}
            <button type="button" role="option" aria-selected={selected} class="grouped-row pressable-row" onclick={() => pick(option.value)}>
                <span class="w-4 shrink-0 text-primary">
                    {#if selected}<Icon name="check" class="size-4" strokeWidth={2.5}/>{/if}
                </span>
                <span class="min-w-0 truncate {selected ? 'text-primary' : ''}">{parts.name}</span>
                {#if parts.detail}<span class="row-value shrink-0">{parts.detail}</span>{/if}
            </button>
        {/each}
    </div>
    {#if footer}<p class="section-footer">{footer}</p>{/if}
</section>
