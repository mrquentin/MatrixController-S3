<script lang="ts" generics="T">
    import PageHeader from "$lib/PageHeader.svelte";
    import Icon from "$lib/Icon.svelte";
    import BackButton from "$lib/BackButton.svelte";
    import {splitLabel} from "$lib/format";

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

    function pick(option: T) {
        value = option;
        onBack();
    }
</script>

<BackButton label={backLabel} {onBack}/>

<PageHeader {title}/>

<section>
    <div class="grouped" role="listbox" aria-label={title}>
        {#each options as option (option.value)}
            {@const parts = splitLabel(option.label)}
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
