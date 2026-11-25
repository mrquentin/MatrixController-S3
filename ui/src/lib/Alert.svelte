<script lang="ts">
    import Icon from "$lib/Icon.svelte";

    let { title, message }: { title: string, message: string } = $props();
</script>

<div role="alert" class="flex items-start gap-3 rounded-box bg-error/10 px-4 py-3 text-error">
    <Icon name="warning" class="mt-0.5 size-5 shrink-0"/>
    <div>
        <p class="font-semibold">{title}</p>
        <p class="text-[0.875rem] opacity-80">{message}</p>
    </div>
</div>
