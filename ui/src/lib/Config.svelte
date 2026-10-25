<script lang="ts">
    import {apiRequest, type ApiResponse} from "$lib/api";
    import type {SystemConfig} from "$lib/interfaces";
    import {busyStore} from "$lib/services/busy.svelte";
    import {DEFAULT_DISPLAY_CONFIG, DISPLAY_HEIGHTS, ENDPOINTS, MAX_BRIGHTNESS} from "$lib/config";
    import {onMount} from "svelte";

    let { config, reload } = $props();
    let emptyRes = $state<ApiResponse<null>>({ data: null, error: null, loading: false });

    // svelte-ignore state_referenced_locally
    let modifiedConfig = $state<SystemConfig>({
        ...config.data,
        // Older config.json files have no display section
        display: { ...DEFAULT_DISPLAY_CONFIG, ...config.data?.display }
    });

    async function save() {
        await busyStore.run(async () => {
           let saveConfig : SystemConfig = { ...modifiedConfig };
           console.log(saveConfig);
           emptyRes = await apiRequest<null>(ENDPOINTS.POST_SYSTEM_CONFIG, "POST", saveConfig);
           await reload();
        });
    }

    onMount(() => {});
</script>

<div class="p-2">
    {#if emptyRes.error }
        <div role="alert" class="alert alert-error">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6 shrink-0 stroke-current" fill="none" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 14l2-2m0 0l2-2m-2 2l-2-2m2 2l2 2m7-2a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
            <span>{emptyRes.error}</span>
        </div>
    {/if}

    <div class="flex flex-wrap gap-4 w-full">

        <div class="card shadow-sm w-150 md:w-200 p-0.5">
            <div class="text-base p-2 font-semibold mb-5 flex">
                <span>Configuration</span>
            </div>

            <div class="p-2">

                <label class="label mb-2">
                    <input type="checkbox" bind:checked={modifiedConfig.debug} class="checkbox checkbox-info" />
                    Debug
                </label>

                <div class="divider text-sm">Matrix</div>

                <div class="flex gap-4 mb-2">
                    <label class="form-control w-full">
                        <span class="label mb-1">Width (px)</span>
                        <input type="number" min="16" max="256" step="1" bind:value={modifiedConfig.display.width} class="input input-sm w-full" />
                    </label>
                    <label class="form-control w-full">
                        <span class="label mb-1">Height (px)</span>
                        <select bind:value={modifiedConfig.display.height} class="select select-sm w-full">
                            {#each DISPLAY_HEIGHTS as height}
                                <option value={height}>{height}</option>
                            {/each}
                        </select>
                    </label>
                </div>

                <label class="form-control w-full mb-2">
                    <span class="label mb-1">Brightness ({modifiedConfig.display.brightness})</span>
                    <input type="range" min="0" max={MAX_BRIGHTNESS} bind:value={modifiedConfig.display.brightness} class="range range-info range-sm" />
                </label>

                <label class="label mb-2">
                    <input type="checkbox" bind:checked={modifiedConfig.display.doubleBuffer} class="checkbox checkbox-info" />
                    Double buffering
                </label>

                <p class="text-xs opacity-60 mb-5">Brightness applies on save. Size and double buffering take effect after a reboot.</p>


                <button class="btn btn-md btn-soft btn-info w-full" onclick={save}>
                    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" fill="currentColor" class="size-5">
                        <path d="M2 3a1 1 0 0 1 1-1h10a1 1 0 0 1 1 1v1a1 1 0 0 1-1 1H3a1 1 0 0 1-1-1V3Z" />
                        <path fill-rule="evenodd" d="M13 6H3v6a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2V6ZM8.75 7.75a.75.75 0 0 0-1.5 0v2.69L6.03 9.22a.75.75 0 0 0-1.06 1.06l2.5 2.5a.75.75 0 0 0 1.06 0l2.5-2.5a.75.75 0 1 0-1.06-1.06l-1.22 1.22V7.75Z" clip-rule="evenodd" />
                    </svg>
                    Save
                </button>

            </div>

        </div>
    </div>
</div>