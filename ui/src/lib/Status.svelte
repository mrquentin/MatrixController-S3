<script lang="ts">
    import type { SystemStatus } from "$lib/interfaces"
    import {apiRequest, type ApiResponse} from "$lib/api";
    import {busyStore} from "$lib/services/busy.svelte";
    import {ENDPOINTS, ORIENTATION_LABELS} from "$lib/config";
    import {formatBytes} from "$lib/format";
    import Alert from "$lib/Alert.svelte";
    import {onMount} from "svelte";
    import PageHeader from "$lib/PageHeader.svelte";
    import Icon from "$lib/Icon.svelte";

    let status = $state<ApiResponse<SystemStatus>>({error: null, data: null});
    let refreshing = $state(false);

    let uptime = $derived.by(() => {
        if (!status.data) return '';
        const total = status.data.uptimeSeconds;
        const d = Math.floor(total / 86400);
        const h = Math.floor((total % 86400) / 3600).toString().padStart(2, '0');
        const m = Math.floor((total % 3600) / 60).toString().padStart(2, '0');
        const s = (total % 60).toString().padStart(2, '0');
        return `${d > 0 ? `${d}d ` : ''}${h}:${m}:${s}`;
    })

    let psramUsed = $derived(status.data && status.data.psramSize > 0
        ? (status.data.psramSize - status.data.psramFree) / status.data.psramSize : 0);


    function formatG(value: number | undefined): string {
        return `${(value ?? 0) >= 0 ? '+' : '−'}${Math.abs(value ?? 0).toFixed(2)}`;
    }

    function humanize(value: string): string {
        const text = value.replaceAll('_', ' ').toLowerCase();
        return text.charAt(0).toUpperCase() + text.slice(1);
    }

    async function load() {
        refreshing = true;
        await busyStore.run(async () => {
            status = await apiRequest<SystemStatus>(ENDPOINTS.SYSTEM_STATUS);
        });
        refreshing = false;
    }

    onMount(() => {
        load();

        const interval = setInterval(() => {
            if (status.data) status.data.uptimeSeconds++;
        }, 1000);

        return () => clearInterval(interval);
    })
</script>

<PageHeader title="Status">
    {#snippet actions()}
        <button class="btn btn-sm btn-ghost pressable gap-1.5 rounded-full text-primary" onclick={load} disabled={refreshing}>
            <Icon name="refresh" class="size-4 {refreshing ? 'animate-spin' : ''}" strokeWidth={2}/>
            Refresh
        </button>
    {/snippet}
</PageHeader>

{#if status.error}
    <div class="mb-6"><Alert title="Couldn't reach the controller" message={status.error}/></div>
{/if}

{#if status.data}
    {@const data = status.data}
    <div class="grid items-start gap-8 md:grid-cols-2 md:gap-6">

        <section>
            <h2 class="section-header">Wi-Fi</h2>
            <div class="grouped">
                <div class="grouped-row">
                    <span>Network</span>
                    <span class="row-value flex items-center justify-end gap-2">
                        <span class="size-2 rounded-full {data.wifiConnected ? 'bg-success' : 'bg-error'}"></span>
                        {data.wifiConnected ? 'Connected' : 'Disconnected'}
                    </span>
                </div>
                <div class="grouped-row">
                    <span>Signal</span>
                    <span class="row-value flex items-center justify-end gap-2">
                        <span class="flex h-3 items-end gap-0.5" aria-hidden="true">
                            {#each [25, 50, 75, 100] as threshold, i}
                                <span class="w-[3px] rounded-[1px] {(data.wifiQuality ?? 0) >= threshold - 12 ? 'bg-base-content/70' : 'bg-base-content/15'}"
                                      style="height: {(i + 1) * 25}%"></span>
                            {/each}
                        </span>
                        {data.wifiQuality}% · {data.wifiRssi} dBm
                    </span>
                </div>
                <div class="grouped-row"><span>IP address</span><span class="row-value select-all">{data.wifiIp}</span></div>
                <div class="grouped-row"><span>Hostname</span><span class="row-value select-all">{data.wifiMDNSName}.local</span></div>
                <div class="grouped-row"><span>Channel</span><span class="row-value">{data.wifiChannel}</span></div>
                <div class="grouped-row"><span>Gateway</span><span class="row-value select-all">{data.wifiGatewayIp}</span></div>
                <div class="grouped-row"><span>MAC address</span><span class="row-value select-all font-mono text-[0.8125rem]">{data.wifiMACAddress}</span></div>
            </div>
        </section>

        <div class="grid gap-8 md:gap-6">
            <section>
                <h2 class="section-header">System</h2>
                <div class="grouped">
                    <div class="grouped-row"><span>Chip</span><span class="row-value">{data.chipModel}</span></div>
                    <div class="grouped-row"><span>Firmware</span><span class="row-value select-text">{data.firmwareVersion}</span></div>
                    <div class="grouped-row"><span>Processor</span><span class="row-value">{data.cpuFreqMHz} MHz</span></div>
                    <div class="grouped-row"><span>Uptime</span><span class="row-value">{uptime}</span></div>
                    <div class="grouped-row">
                        <span>Last reset</span>
                        <span class="row-value {data.resetFault ? 'font-medium text-warning' : ''}">{humanize(data.resetReason)}</span>
                    </div>
                </div>
            </section>

            <section>
                <h2 class="section-header">Memory</h2>
                <div class="grouped">
                    <div class="grouped-row"><span>Free heap</span><span class="row-value">{formatBytes(data.memoryFreeHeap)}</span></div>
                    <div class="grouped-row"><span>Largest block</span><span class="row-value">{formatBytes(data.memoryMaxBlock)}</span></div>
                    <div class="grouped-row"><span>Lowest free heap</span><span class="row-value">{formatBytes(data.memoryMinFreeHeap)}</span></div>
                    <div class="grouped-row flex-col items-stretch gap-2 py-3">
                        <div class="flex items-center">
                            <span>PSRAM</span>
                            <span class="row-value">{formatBytes(data.psramSize - data.psramFree)} of {formatBytes(data.psramSize)}</span>
                        </div>
                        <progress class="progress progress-primary h-1.5" value={psramUsed} max="1"></progress>
                    </div>
                </div>
            </section>

            {#if data.orientation}
                <section>
                    <h2 class="section-header">Orientation</h2>
                    <div class="grouped">
                        <div class="grouped-row">
                            <span>Accelerometer</span>
                            <span class="row-value {data.orientation.sensor ? '' : 'font-medium text-warning'}">{data.orientation.sensor ? 'Found' : 'Not found'}</span>
                        </div>
                        <div class="grouped-row">
                            <span>Showing</span>
                            <span class="row-value">{data.orientation.upsideDown ? 'Upside down' : 'Upright'} · {ORIENTATION_LABELS[data.orientation.mode] ?? data.orientation.mode}</span>
                        </div>
                        {#if data.orientation.sensor}
                            <!-- The axis pointing up reads about +1 g: tells which one runs along the panel's height -->
                            <div class="grouped-row">
                                <span>Gravity</span>
                                <span class="row-value font-mono text-[0.8125rem] tabular-nums">
                                    x {formatG(data.orientation.x)} · y {formatG(data.orientation.y)} · z {formatG(data.orientation.z)} g
                                </span>
                            </div>
                        {/if}
                    </div>
                </section>
            {/if}
        </div>

    </div>
{/if}
