<script lang="ts">
    import Icon from "$lib/Icon.svelte";

    let { label, onBack }: { label: string, onBack: () => void } = $props();
</script>

<button type="button" class="pressable -ml-1.5 mb-3 flex items-center gap-0.5 rounded-md py-1 pr-2 text-[1.0625rem] text-primary" onclick={onBack}>
    <Icon name="back" class="size-5" strokeWidth={2.5}/> {label}
</button>
