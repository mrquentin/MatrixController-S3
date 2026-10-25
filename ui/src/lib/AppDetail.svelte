<script lang="ts">
    import type {AppConfig, AppField, AppInfo} from "$lib/interfaces";
    import {apiRequest} from "$lib/api";
    import {busyStore} from "$lib/services/busy.svelte";
    import {ENDPOINTS} from "$lib/config";
    import PageHeader from "$lib/PageHeader.svelte";
    import SegmentedControl from "$lib/SegmentedControl.svelte";
    import SaveBar from "$lib/SaveBar.svelte";
    import Icon from "$lib/Icon.svelte";

    // Form generated from the app's schema, so new firmware apps need no UI code
    let { app, active, onActivate, onBack, onSaved }: {
        app: AppInfo,
        active: boolean,
        onActivate: () => void,
        onBack: () => void,
        onSaved: (config: AppConfig) => void
    } = $props();

    // svelte-ignore state_referenced_locally
    let values = $state<AppConfig>({ ...app.config });
    let error = $state<string | null>(null);

    // The board writes colors as "#FF8800" while the picker reports "#ff8800"
    function same(field: AppField, a: unknown, b: unknown): boolean {
        return field.type === 'color' ? String(a).toLowerCase() === String(b).toLowerCase() : a === b;
    }

    function fieldError(field: AppField): string | null {
        if (field.type !== 'number') return null;
        const value = values[field.key];
        if (typeof value !== 'number' || !Number.isInteger(value) || value < field.min || value > field.max) {
            return `${field.label} must be a whole number from ${field.min} to ${field.max}.`;
        }
        return null;
    }

    let dirty = $derived(app.schema.some(f => !same(f, values[f.key], app.config[f.key])));
    let errors = $derived(app.schema.map(fieldError).filter(e => e !== null));

    function step(field: AppField & { type: 'number' }, delta: number) {
        const current = typeof values[field.key] === 'number' ? values[field.key] as number : field.min;
        values[field.key] = Math.min(field.max, Math.max(field.min, Math.round(current) + delta));
    }

    async function save(): Promise<boolean> {
        error = null;
        return busyStore.run(async () => {
            const res = await apiRequest<{ config: AppConfig }>(ENDPOINTS.POST_APP_CONFIG, "POST", { id: app.id, config: values });
            if (!res.data) {
                error = res.error;
                return false;
            }
            // Show what the board applied: it corrects out-of-range values
            values = { ...res.data.config };
            onSaved(res.data.config);
            return true;
        });
    }

    function revert() {
        values = { ...app.config };
        error = null;
    }
</script>

<button type="button" class="pressable -ml-1.5 mb-3 flex items-center gap-0.5 rounded-md py-1 pr-2 text-[1.0625rem] text-primary" onclick={onBack}>
    <Icon name="back" class="size-5" strokeWidth={2.5}/> Apps
</button>

<PageHeader title={app.name} subtitle={active ? 'Showing on the panel' : undefined}/>

<div class="grid gap-8">

    <div class="grouped">
        {#if active}
            <div class="grouped-row text-success">
                <Icon name="check" class="size-5" strokeWidth={2.5}/>
                <span class="font-medium">On the panel now</span>
            </div>
        {:else}
            <button type="button" class="grouped-row pressable-row font-medium text-primary transition-colors" onclick={onActivate}>
                <Icon name="play" class="size-5" strokeWidth={2}/>
                Show on panel
            </button>
        {/if}
    </div>

    {#if app.schema.length > 0}
        <section>
            <h2 class="section-header">Settings</h2>
            <div class="grouped">
                {#each app.schema as field (field.key)}
                    {#if field.type === 'bool'}
                        <label class="grouped-row cursor-pointer">
                            <span>{field.label}</span>
                            <input type="checkbox" bind:checked={values[field.key] as boolean} class="switch ml-auto"/>
                        </label>
                    {:else if field.type === 'text'}
                        <label class="grouped-row flex-col items-stretch gap-1 py-3">
                            <span class="flex items-baseline">
                                {field.label}
                                <span class="ml-auto text-[0.75rem] text-base-content/40 tabular-nums">
                                    {String(values[field.key] ?? '').length}/{field.maxLength}
                                </span>
                            </span>
                            {#if field.maxLength > 64}
                                <textarea rows="2" maxlength={field.maxLength} bind:value={values[field.key]}
                                          class="field-sizing-content min-h-12 resize-none bg-transparent text-base-content/70 outline-none focus:text-base-content"></textarea>
                            {:else}
                                <input type="text" maxlength={field.maxLength} bind:value={values[field.key]}
                                       class="bg-transparent text-base-content/70 outline-none focus:text-base-content"/>
                            {/if}
                        </label>
                    {:else if field.type === 'number'}
                        {@const invalid = fieldError(field) !== null}
                        <div class="grouped-row">
                            <label for="field-{field.key}" class="min-w-0 truncate">{field.label}</label>
                            <input id="field-{field.key}" type="number" inputmode="numeric" min={field.min} max={field.max} step="1"
                                   bind:value={values[field.key]} aria-invalid={invalid}
                                   class="ml-auto w-14 bg-transparent text-right tabular-nums outline-none focus:text-base-content {invalid ? 'text-error' : 'text-base-content/55'}"/>
                            <!-- iOS-style stepper -->
                            <span class="stepper flex h-8 shrink-0 items-center overflow-hidden rounded-[0.5rem]">
                                <button type="button" class="pressable flex h-full w-10 items-center justify-center disabled:opacity-30"
                                        aria-label="Decrease {field.label}" disabled={values[field.key] as number <= field.min}
                                        onclick={() => step(field, -1)}>
                                    <Icon name="minus" class="size-4" strokeWidth={2.5}/>
                                </button>
                                <span class="h-4 w-px bg-base-content/15"></span>
                                <button type="button" class="pressable flex h-full w-10 items-center justify-center disabled:opacity-30"
                                        aria-label="Increase {field.label}" disabled={values[field.key] as number >= field.max}
                                        onclick={() => step(field, 1)}>
                                    <Icon name="plus" class="size-4" strokeWidth={2.5}/>
                                </button>
                            </span>
                        </div>
                    {:else if field.type === 'color'}
                        <label class="grouped-row cursor-pointer">
                            <span>{field.label}</span>
                            <span class="ml-auto font-mono text-[0.8125rem] uppercase text-base-content/55">{values[field.key]}</span>
                            <input type="color" bind:value={values[field.key]} class="swatch"/>
                        </label>
                    {:else if field.type === 'select'}
                        {#if field.options.length <= 3}
                            <div class="grouped-row">
                                <span class="shrink-0">{field.label}</span>
                                <div class="ml-auto w-48">
                                    <SegmentedControl label={field.label} options={field.options} bind:value={values[field.key] as string}/>
                                </div>
                            </div>
                        {:else}
                            <label class="grouped-row cursor-pointer">
                                <span class="shrink-0">{field.label}</span>
                                <span class="ml-auto flex min-w-0 items-center gap-1 text-base-content/55">
                                    <select bind:value={values[field.key]}
                                            class="min-w-0 cursor-pointer appearance-none truncate bg-transparent text-right outline-none focus-visible:text-primary">
                                        {#each field.options as option (option.value)}
                                            <option value={option.value}>{option.label}</option>
                                        {/each}
                                    </select>
                                    <Icon name="chevron" class="size-3.5 shrink-0 rotate-90 opacity-60" strokeWidth={2.5}/>
                                </span>
                            </label>
                        {/if}
                    {/if}
                {/each}
            </div>
            {#each errors as message}
                <p class="section-footer text-error!">{message}</p>
            {/each}
        </section>

        <SaveBar {dirty} valid={errors.length === 0} {error} errorTitle="Settings weren't saved" onSave={save} onRevert={revert}/>
    {:else}
        <p class="section-footer -mt-6">This app has no settings.</p>
    {/if}

</div>

<style>
    .stepper {
        background: light-dark(rgb(118 118 128 / 0.12), rgb(118 118 128 / 0.24));
    }

    /* Native color picker shown as a round swatch */
    .swatch {
        width: 1.75rem;
        height: 1.75rem;
        padding: 0;
        border: none;
        border-radius: 9999px;
        background: none;
        cursor: pointer;
        flex-shrink: 0;
    }
    .swatch::-webkit-color-swatch-wrapper {
        padding: 0;
    }
    .swatch::-webkit-color-swatch {
        border: none;
        border-radius: 9999px;
        box-shadow: inset 0 0 0 1px rgb(0 0 0 / 0.1);
    }
    .swatch::-moz-color-swatch {
        border: none;
        border-radius: 9999px;
    }
</style>
