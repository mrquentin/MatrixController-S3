<script lang="ts">
    import {apiRequest} from "$lib/api";
    import {busyStore} from "$lib/services/busy.svelte";
    import {ENDPOINTS} from "$lib/config";
    import type {WifiNetwork} from "$lib/interfaces";
    import PageHeader from "$lib/PageHeader.svelte";
    import ConfirmDialog from "$lib/ConfirmDialog.svelte";
    import Icon from "$lib/Icon.svelte";

    // Picks a new network; onJoin runs once the user has confirmed the restart
    let { current, onBack, onJoin }: {
        current: string,
        onBack: () => void,
        onJoin: (ssid: string, password: string) => void
    } = $props();

    let passwordInput = $state<HTMLInputElement>();
    let networks = $state<WifiNetwork[] | null>(null);
    let ssid = $state('');
    let password = $state('');
    let showPassword = $state(false);
    let confirming = $state(false);

    // Same limits as WifiStore::isValid on the board, which counts bytes
    const bytes = (text: string) => new TextEncoder().encode(text).length;
    let ssidValid = $derived(bytes(ssid) >= 1 && bytes(ssid) <= 32);
    let passwordValid = $derived(password.length === 0 || (bytes(password) >= 8 && bytes(password) <= 63));

    // The board scans in the background and answers with an empty list until the first scan is done
    $effect(() => {
        let timer: ReturnType<typeof setTimeout>;
        let stopped = false;
        const scan = async () => {
            const response = await apiRequest<WifiNetwork[]>(ENDPOINTS.GET_WIFI_SCAN);
            if (stopped) return;
            if (response.data?.length) networks = response.data;
            // Keep the list fresh while the page stays open
            timer = setTimeout(scan, response.data?.length ? 10000 : 2000);
        };
        scan();
        return () => {
            stopped = true;
            clearTimeout(timer);
        };
    });

    function pick(network: WifiNetwork) {
        ssid = network.ssid;
        password = '';
        if (network.secure) passwordInput?.focus();
    }
</script>

<button type="button" class="pressable -ml-1.5 mb-3 flex items-center gap-0.5 rounded-md py-1 pr-2 text-[1.0625rem] text-primary" onclick={onBack}>
    <Icon name="back" class="size-5" strokeWidth={2.5}/> Settings
</button>

<PageHeader title="Change WiFi" subtitle={current ? `Currently on ${current}` : undefined}/>

<div class="grid gap-8">

    <section>
        <h2 class="section-header flex items-center gap-2">
            Networks
            {#if !networks}<span class="loading loading-spinner loading-xs"></span>{/if}
        </h2>
        <div class="grouped">
            {#if networks}
                {#each networks as network (network.ssid)}
                    <button type="button" class="grouped-row pressable-row" onclick={() => pick(network)}>
                        <span class="w-4 shrink-0 text-primary">
                            {#if network.ssid === ssid}<Icon name="check" class="size-4" strokeWidth={2.5}/>{/if}
                        </span>
                        <span class="min-w-0 truncate">{network.ssid}</span>
                        {#if network.ssid === current}
                            <span class="shrink-0 text-[0.8125rem] text-base-content/55">Current</span>
                        {/if}
                        <span class="row-value flex shrink-0 items-center justify-end gap-1.5">
                            {#if network.secure}<Icon name="lock" class="size-3.5"/>{/if}
                            {network.quality}%
                        </span>
                    </button>
                {/each}
            {:else}
                <div class="grouped-row text-base-content/55">Looking for networks...</div>
            {/if}
        </div>
    </section>

    <section>
        <h2 class="section-header">Network to join</h2>
        <div class="grouped">
            <label class="grouped-row cursor-text">
                <span class="w-24 shrink-0">Name</span>
                <input bind:value={ssid} maxlength="32" autocapitalize="off" autocomplete="off" spellcheck="false"
                       class="min-w-0 flex-1 bg-transparent outline-none" placeholder="Pick one above or type it"/>
            </label>
            <label class="grouped-row cursor-text">
                <span class="w-24 shrink-0">Password</span>
                <input bind:this={passwordInput} bind:value={password} type={showPassword ? 'text' : 'password'} maxlength="63"
                       autocapitalize="off" autocomplete="off" spellcheck="false"
                       class="min-w-0 flex-1 bg-transparent outline-none" placeholder="None for open networks"/>
                <button type="button" class="shrink-0 text-[0.8125rem] text-primary" onclick={() => showPassword = !showPassword}>
                    {showPassword ? 'Hide' : 'Show'}
                </button>
            </label>
        </div>
        {#if !passwordValid}
            <p class="section-footer text-error!">WiFi passwords are 8 to 63 characters.</p>
        {:else}
            <p class="section-footer">If the controller can't join this network, it starts its setup network again so you can pick another.</p>
        {/if}
    </section>

    <button class="btn btn-primary pressable h-12 rounded-box text-[1.0625rem] font-semibold shadow-none disabled:bg-base-content/10 disabled:text-base-content/35"
            disabled={!ssidValid || !passwordValid || busyStore.active} onclick={() => confirming = true}>
        Join network
    </button>

</div>

<ConfirmDialog bind:open={confirming} title="Join {ssid}?"
               message="The controller restarts to join this network. If it gets a new address there, reopen this page at http://matrixcontroller.local."
               confirmLabel="Restart" onConfirm={() => onJoin(ssid, password)}/>
