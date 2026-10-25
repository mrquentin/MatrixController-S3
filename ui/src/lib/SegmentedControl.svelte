<script lang="ts" generics="T extends string | number">
    import {prefersReducedMotion, Spring} from "svelte/motion";
    import {SPRING_DEFAULT} from "$lib/motion";

    let { options, value = $bindable(), label }: {
        options: { value: T, label: string }[],
        value: T,
        label: string
    } = $props();

    let index = $derived(Math.max(0, options.findIndex(o => o.value === value)));

    // The thumb springs from wherever it currently is, so rapid taps redirect it mid-flight
    // svelte-ignore state_referenced_locally
    const position = new Spring(index, SPRING_DEFAULT);

    $effect(() => {
        position.set(index, { instant: prefersReducedMotion.current });
    });

    function onkeydown(e: KeyboardEvent) {
        const step = e.key === 'ArrowRight' || e.key === 'ArrowDown' ? 1
            : e.key === 'ArrowLeft' || e.key === 'ArrowUp' ? -1 : 0;
        if (!step) return;
        e.preventDefault();
        const next = (index + step + options.length) % options.length;
        value = options[next].value;
        ((e.currentTarget as HTMLElement).children[next + 1] as HTMLElement)?.focus();
    }
</script>

<div role="radiogroup" aria-label={label} tabindex="-1" {onkeydown}
     class="segmented relative grid w-full rounded-[0.5625rem] p-0.5"
     style="grid-template-columns: repeat({options.length}, minmax(0, 1fr))">
    <div aria-hidden="true"
         class="thumb pointer-events-none absolute top-0.5 bottom-0.5 left-0.5 rounded-[0.4375rem] will-change-transform"
         style="width: calc((100% - 0.25rem) / {options.length}); transform: translateX({position.current * 100}%)"></div>
    {#each options as option, i (option.value)}
        <button type="button" role="radio" aria-checked={i === index} tabindex={i === index ? 0 : -1}
                class="relative z-10 h-7 truncate px-3 text-[0.8125rem] {i === index ? 'font-semibold' : 'font-medium text-base-content/80'}"
                onclick={() => value = option.value}>
            {option.label}
        </button>
    {/each}
</div>

<style>
    .segmented {
        background: light-dark(rgb(118 118 128 / 0.12), rgb(118 118 128 / 0.24));
    }
    .thumb {
        background: light-dark(#ffffff, #636366);
        box-shadow: 0 3px 8px rgb(0 0 0 / 0.12), 0 3px 1px rgb(0 0 0 / 0.04), 0 0 0 0.5px rgb(0 0 0 / 0.04);
    }
    button:focus-visible {
        outline: 2px solid var(--color-primary);
        outline-offset: -2px;
        border-radius: 0.4375rem;
    }
</style>
