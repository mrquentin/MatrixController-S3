<script lang="ts">
    import {apiRequest, errorDetails} from "$lib/api";
    import {busyStore} from "$lib/services/busy.svelte";
    import {ENDPOINTS, UPDATE_PASSWORD_HEADER, UPDATE_PASSWORD_MAX, UPDATE_PASSWORD_MIN} from "$lib/config";
    import type {UpdateStatus} from "$lib/interfaces";
    import PageHeader from "$lib/PageHeader.svelte";
    import ConfirmDialog from "$lib/ConfirmDialog.svelte";
    import Icon from "$lib/Icon.svelte";

    // Firmware version, update password and installing updates. onInstalled runs once the board
    // has an update in and restarts into it
    let { status, reload, onBack, onInstalled }: {
        status: UpdateStatus | null,
        reload: () => Promise<void>,
        onBack: () => void,
        onInstalled: (version: string | null) => void
    } = $props();

    // Update password: a first one, or a new one replacing it
    let currentPassword = $state('');
    let newPassword = $state('');
    let showPasswords = $state(false);
    let passwordError = $state<string | null>(null);
    let passwordSaved = $state(false);
    let changingPassword = $state(false);
    let newPasswordValid = $derived(newPassword.length >= UPDATE_PASSWORD_MIN && newPassword.length <= UPDATE_PASSWORD_MAX);

    // Installing: the password goes with every update request
    let password = $state('');
    let file = $state<File | null>(null);
    let fileInput = $state<HTMLInputElement>();
    let confirmUpload = $state(false);
    let confirmRelease = $state(false);
    let installError = $state<string | null>(null);
    // 0-1 while this page is uploading a file
    let uploadProgress = $state<number | null>(null);

    // Largest image that fits an app slot of partitions-8MB-tinyuf2.csv
    const MAX_IMAGE_SIZE = 2 * 1024 * 1024;
    let fileProblem = $derived(!file ? null
        : !file.name.toLowerCase().endsWith('.bin') ? 'Pick the firmware.bin of a build'
        : file.size > MAX_IMAGE_SIZE ? 'This file is too large for the controller' : null);

    let running = $derived(status?.state === 'running');
    let releaseBusy = $derived(!!status?.release?.busy);
    let installing = $derived(uploadProgress !== null || running || (releaseBusy && status?.release?.job === 'install'));
    let canInstall = $derived(!!status?.passwordSet && password.length > 0 && !installing && status?.state !== 'installed');

    // Progress of updates the board runs itself (release downloads, PlatformIO) comes from polling
    let boardProgress = $derived(status && status.size > 0 ? status.written / status.size : null);
    let progress = $derived(uploadProgress ?? (running ? boardProgress : null));

    // Polls so progress and results show: often while the board is busy with an update or a release
    // check, now and then otherwise, which picks up uploads started from PlatformIO
    $effect(() => {
        const timer = setInterval(reload, running || releaseBusy ? 1000 : 5000);
        return () => clearInterval(timer);
    });

    // Release installs and PlatformIO uploads finish on the board, this page just sees the result
    let reportedInstall = false;
    $effect(() => {
        if (status?.state === 'installed' && uploadProgress === null && !reportedInstall) {
            reportedInstall = true;
            onInstalled(status.source === 'release' ? status.release?.latest ?? null : null);
        }
    });

    function formatSize(bytes: number): string {
        return bytes >= 1024 * 1024 ? `${(bytes / 1024 / 1024).toFixed(2)} MB` : `${Math.round(bytes / 1024)} KB`;
    }

    function formatDate(iso: string | undefined): string {
        if (!iso) return '';
        const date = new Date(iso);
        return isNaN(date.getTime()) ? '' : date.toLocaleDateString(undefined, { day: 'numeric', month: 'short', year: 'numeric' });
    }

    async function savePassword() {
        passwordError = null;
        passwordSaved = false;
        const response = await busyStore.run(() => apiRequest<null>(ENDPOINTS.POST_UPDATE_PASSWORD, "POST",
            { current: currentPassword, password: newPassword }));
        if (response.error) {
            passwordError = response.error;
            return;
        }
        // The new password is the one to install with from now on
        password = newPassword;
        currentPassword = '';
        newPassword = '';
        changingPassword = false;
        passwordSaved = true;
        await reload();
    }

    // Asks before a long upload, which the board would otherwise only refuse at the end
    async function verifyPassword(): Promise<boolean> {
        const response = await apiRequest<null>(ENDPOINTS.POST_UPDATE_VERIFY, "POST", undefined,
            { [UPDATE_PASSWORD_HEADER]: password });
        installError = response.error;
        return !response.error;
    }

    // XMLHttpRequest rather than fetch: only it reports upload progress
    function send(image: File): Promise<string | null> {
        return new Promise(resolve => {
            const request = new XMLHttpRequest();
            request.open('POST', ENDPOINTS.POST_UPDATE);
            // Anything else (e.g. a form encoding) the board would try to parse instead of flashing
            request.setRequestHeader('Content-Type', 'application/octet-stream');
            request.setRequestHeader(UPDATE_PASSWORD_HEADER, password);
            request.upload.onprogress = event => {
                if (event.lengthComputable) uploadProgress = event.loaded / event.total;
            };
            request.onload = () => resolve(request.status === 200 ? null : errorDetails(request.responseText, request.status));
            request.onerror = () => resolve('The upload was interrupted');
            request.send(image);
        });
    }

    async function upload() {
        if (!file) return;
        installError = null;
        if (!await busyStore.run(verifyPassword)) return;

        uploadProgress = 0;
        const error = await busyStore.run(() => send(file!));
        uploadProgress = null;
        if (error) {
            installError = error;
            await reload();
            return;
        }
        reportedInstall = true;
        onInstalled(null);
    }

    async function checkRelease() {
        installError = null;
        const response = await busyStore.run(() => apiRequest<null>(ENDPOINTS.POST_RELEASE_CHECK, "POST"));
        if (response.error) installError = response.error;
        await reload();
    }

    async function installRelease() {
        installError = null;
        const response = await busyStore.run(() => apiRequest<null>(ENDPOINTS.POST_RELEASE_INSTALL, "POST", undefined,
            { [UPDATE_PASSWORD_HEADER]: password }));
        if (response.error) installError = response.error;
        await reload();
    }

    function pickFile(event: Event) {
        file = (event.currentTarget as HTMLInputElement).files?.[0] ?? null;
        installError = null;
    }
</script>

<button type="button" class="pressable -ml-1.5 mb-3 flex items-center gap-0.5 rounded-md py-1 pr-2 text-[1.0625rem] text-primary" onclick={onBack}>
    <Icon name="back" class="size-5" strokeWidth={2.5}/> Settings
</button>

<PageHeader title="Firmware" subtitle={status ? `Version ${status.version}` : undefined}/>

{#if !status}
    <p class="text-base-content/55">Loading...</p>
{:else}
<div class="grid gap-8">

    {#if status.rolledBack}
        <div role="status" class="flex items-start gap-3 rounded-box bg-warning/15 px-4 py-3">
            <Icon name="warning" class="mt-0.5 size-5 shrink-0 text-warning"/>
            <div>
                <p class="font-semibold">The last update was undone</p>
                <p class="text-[0.875rem] text-base-content/70">It didn't start up properly, so the controller went back to this version.</p>
            </div>
        </div>
    {/if}

    <section>
        <h2 class="section-header">Installed</h2>
        <div class="grouped">
            <div class="grouped-row">
                <span>Version</span>
                <span class="row-value select-text">{status.version}</span>
            </div>
            <div class="grouped-row">
                <span>App slot</span>
                <span class="row-value">{status.slot}</span>
            </div>
        </div>
        {#if status.onProbation}
            <p class="section-footer">Just updated: if this version crashes or restarts in its first 30 seconds, the controller goes back to the previous one.</p>
        {/if}
    </section>

    <section>
        <h2 class="section-header">Update password</h2>
        {#if status.passwordSet && !changingPassword}
            <div class="grouped">
                <div class="grouped-row">
                    <span>Password</span>
                    <span class="row-value">{passwordSaved ? 'Saved' : 'Set'}</span>
                </div>
                <button type="button" class="grouped-row pressable-row text-primary" onclick={() => { changingPassword = true; passwordSaved = false; }}>
                    Change password
                </button>
            </div>
        {:else}
            <div class="grouped">
                {#if status.passwordSet}
                    <label class="grouped-row cursor-text">
                        <span class="w-24 shrink-0">Current</span>
                        <input bind:value={currentPassword} type={showPasswords ? 'text' : 'password'} autocomplete="current-password"
                               class="min-w-0 flex-1 bg-transparent outline-none" placeholder="Current password"/>
                    </label>
                {/if}
                <label class="grouped-row cursor-text">
                    <span class="w-24 shrink-0">{status.passwordSet ? 'New' : 'Password'}</span>
                    <input bind:value={newPassword} type={showPasswords ? 'text' : 'password'} maxlength={UPDATE_PASSWORD_MAX}
                           autocomplete="new-password" class="min-w-0 flex-1 bg-transparent outline-none"
                           placeholder="{UPDATE_PASSWORD_MIN} characters or more"/>
                    <button type="button" class="shrink-0 text-[0.8125rem] text-primary" onclick={() => showPasswords = !showPasswords}>
                        {showPasswords ? 'Hide' : 'Show'}
                    </button>
                </label>
                <button type="button" class="grouped-row pressable-row text-primary disabled:opacity-40"
                        disabled={!newPasswordValid || (status.passwordSet && !currentPassword) || busyStore.active} onclick={savePassword}>
                    {status.passwordSet ? 'Change password' : 'Set password'}
                </button>
                {#if status.passwordSet}
                    <button type="button" class="grouped-row pressable-row" onclick={() => { changingPassword = false; passwordError = null; }}>
                        Cancel
                    </button>
                {/if}
            </div>
        {/if}
        {#if passwordError}
            <p class="section-footer text-error!">{passwordError}</p>
        {:else if !status.passwordSet}
            <p class="section-footer">Updates are refused until a password is set. Anyone on your network who knows it can replace the firmware.</p>
        {:else}
            <p class="section-footer">Needed for every update, including flashing from PlatformIO over WiFi. Forgotten? Erasing the flash over USB clears it, along with the WiFi settings.</p>
        {/if}
    </section>

    {#if status.passwordSet}
        <section>
            <h2 class="section-header">Install from a file</h2>
            <div class="grouped">
                <label class="grouped-row cursor-text">
                    <span class="w-24 shrink-0">Password</span>
                    <input bind:value={password} type="password" autocomplete="current-password"
                           class="min-w-0 flex-1 bg-transparent outline-none" placeholder="Update password"/>
                </label>
                <button type="button" class="grouped-row pressable-row" disabled={installing} onclick={() => fileInput?.click()}>
                    <span class="shrink-0">File</span>
                    <span class="row-value truncate">{file ? `${file.name}, ${formatSize(file.size)}` : 'Choose...'}</span>
                    <Icon name="chevron" class="size-4 shrink-0 text-base-content/30" strokeWidth={2.5}/>
                </button>
                <input bind:this={fileInput} type="file" accept=".bin,application/octet-stream" class="hidden" onchange={pickFile}/>
            </div>
            {#if fileProblem}
                <p class="section-footer text-error!">{fileProblem}</p>
            {:else}
                <p class="section-footer">The firmware.bin from a build, in .pio/build/adafruit_matrixportal_esp32s3. Your settings and WiFi are kept.</p>
            {/if}
        </section>

        {#if progress !== null || installing}
            <section aria-live="polite">
                <div class="mb-2 flex items-center text-[0.9375rem]">
                    <span>{status.source === 'release' ? 'Downloading and installing' : status.source === 'network' ? 'Flashing from PlatformIO' : 'Uploading'}</span>
                    {#if progress !== null}<span class="ml-auto tabular-nums text-base-content/55">{Math.round(progress * 100)}%</span>{/if}
                </div>
                <progress class="progress progress-primary w-full" value={progress !== null ? progress * 100 : undefined} max="100"></progress>
                <p class="section-footer">Keep the controller powered. The panel shows the progress too.</p>
            </section>
        {/if}

        <button class="btn btn-primary pressable h-12 rounded-box text-[1.0625rem] font-semibold shadow-none disabled:bg-base-content/10 disabled:text-base-content/35"
                disabled={!canInstall || !file || !!fileProblem || busyStore.active} onclick={() => confirmUpload = true}>
            Install file
        </button>

        {#if status.release}
            {@const release = status.release}
            <section>
                <h2 class="section-header">Latest release</h2>
                <div class="grouped">
                    <div class="grouped-row">
                        <span class="shrink-0">Repository</span>
                        <span class="row-value truncate">{release.repo}</span>
                    </div>
                    {#if release.latest}
                        <div class="grouped-row">
                            <span class="shrink-0">Latest</span>
                            <span class="row-value truncate">
                                {release.latest}{release.publishedAt ? `, ${formatDate(release.publishedAt)}` : ''}
                            </span>
                        </div>
                    {/if}
                    <button type="button" class="grouped-row pressable-row text-primary disabled:opacity-40"
                            disabled={releaseBusy || installing} onclick={checkRelease}>
                        <span>{releaseBusy && release.job === 'check' ? 'Checking...' : 'Check for updates'}</span>
                    </button>
                    {#if release.latest}
                        <button type="button" class="grouped-row pressable-row text-primary disabled:opacity-40"
                                disabled={!canInstall || releaseBusy} onclick={() => confirmRelease = true}>
                            {release.updateAvailable ? `Install ${release.latest}` : `Reinstall ${release.latest}`}
                        </button>
                    {/if}
                </div>
                {#if release.error}
                    <p class="section-footer text-error!">{release.error}</p>
                {:else if release.latest && !release.updateAvailable}
                    <p class="section-footer">This is the version installed.</p>
                {:else}
                    <p class="section-footer">Installs the firmware.bin attached to the latest release on GitHub, using the password above.</p>
                {/if}
            </section>
        {/if}
    {/if}

    {#if installError || (status.state === 'failed' && status.error)}
        <div role="alert" class="flex items-start gap-3 rounded-box bg-error/10 px-4 py-3">
            <Icon name="warning" class="mt-0.5 size-5 shrink-0 text-error"/>
            <div>
                <p class="font-semibold text-error">Update not installed</p>
                <p class="text-[0.875rem] text-base-content/70">
                    {installError ?? status.error}.
                    {!installError && status.files
                        ? 'The file system was only partly written: upload it again from PlatformIO.'
                        : `The controller carries on with version ${status.version}.`}
                </p>
            </div>
        </div>
    {/if}

    <section>
        <h2 class="section-header">PlatformIO over WiFi</h2>
        <div class="grouped">
            <div class="grouped-row">
                <span>Status</span>
                <span class="row-value">{status.networkUpload.listening ? `Listening on port ${status.networkUpload.port}` : 'Off'}</span>
            </div>
        </div>
        {#if !status.networkUpload.listening}
            <p class="section-footer">Starts once an update password is set.</p>
        {:else if status.networkUpload.restartNeeded}
            <p class="section-footer">Still uses the previous password until the controller restarts.</p>
        {:else}
            <p class="section-footer">Run <code>pio run -e ota -t upload</code> with the update password in MATRIX_UPDATE_PASSWORD.</p>
        {/if}
    </section>

</div>
{/if}

<ConfirmDialog bind:open={confirmUpload} title="Install {file?.name ?? 'this file'}?"
               message="The controller restarts into the new firmware. If it doesn't start properly, it goes back to version {status?.version}."
               confirmLabel="Install" onConfirm={upload}/>

<ConfirmDialog bind:open={confirmRelease} title="Install {status?.release?.latest ?? 'the latest release'}?"
               message="The controller downloads it from GitHub, then restarts into it. If it doesn't start properly, it goes back to version {status?.version}."
               confirmLabel="Install" onConfirm={installRelease}/>
