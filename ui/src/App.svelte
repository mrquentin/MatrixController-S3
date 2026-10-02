<script lang="ts">
    import {onMount} from "svelte";
    import {socketService} from "$lib/services/socket.svelte";
    import {ENDPOINTS} from "$lib/config";
    import Overlay from "$lib/Overlay.svelte";
    import {busyStore} from "$lib/services/busy.svelte";
    import Status from "$lib/Status.svelte";
    import {apiRequest, type ApiResponse} from "$lib/api";
    import type {SystemConfig} from "$lib/interfaces";
    import Config from "$lib/Config.svelte";

    onMount(() => {
        socketService.connect(ENDPOINTS.WEB_SOCKET)
        preflight();
        loadConfig();
    })

    let config = $state<ApiResponse<SystemConfig>>({ data: { debug: true }, error: null, loading: false })

    let activeTab = $state('home');

    function navigate(tab: string){
      activeTab = tab;
      // When navigating, we'll close the open menu (only for mobile as desktop is always open)
      const toggle = document.getElementById('drawer-menu') as HTMLInputElement | null;
      if (toggle) toggle.checked = false;
    }

    function preflight() {
        config.error = "";
    }

    async function loadConfig() {
        await busyStore.run(async () => {
           config = await apiRequest<SystemConfig>(ENDPOINTS.GET_SYSTEM_CONFIG);
        });
    }
</script>

<main>

    <!-- Navbar: outside the drawer so it spans the full width above the sidebar.
         z-20 keeps it above the drawer (z-10) so the toggle stays usable on mobile -->
    <nav class="navbar w-full bg-base-300 sticky top-0 z-20">
        <!-- Desktop keeps the drawer permanently open (lg:drawer-open), so the toggle is mobile-only -->
        <label for="drawer-menu" aria-label="toggle sidebar" class="btn btn-square btn-ghost lg:hidden">
            <!-- Sidebar toggle icon -->
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" stroke-linejoin="round"
                 stroke-linecap="round" stroke-width="2" fill="none" stroke="currentColor"
                 class="my-1.5 inline-block size-4">
                <path d="M4 4m0 2a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2v12a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2z"></path>
                <path d="M9 4v16"></path>
                <path d="M14 10l2 2l-2 2"></path>
            </svg>
        </label>
        <div class="px-4"><h2>Matrix Controller</h2></div>
    </nav>

    <div class="drawer lg:drawer-open">
        <input id="drawer-menu" type="checkbox" class="drawer-toggle"/>
        <div class="drawer-content">
            <!-- Page content here -->
            <div class="p-4 pb-20">
                <Overlay active={busyStore.active}>
                    {#if activeTab === 'home'}
                        <h2>Hello</h2>
                    {:else if activeTab === 'status'}
                        <Status/>
                    {:else if activeTab === 'config'}
                        <Config config={config} reload={loadConfig} />
                    {/if}
                </Overlay>
            </div>
        </div>

        <!-- The drawer starts below the navbar (h-16) instead of covering it -->
        <div class="drawer-side max-lg:is-drawer-close:overflow-visible top-16 h-[calc(100dvh-4rem)]">
            <label for="drawer-menu" aria-label="close sidebar" class="drawer-overlay"></label>
            <div class="flex min-h-full flex-col items-start bg-base-200 w-64 max-lg:is-drawer-close:w-14">
                <!-- Sidebar content here -->
                <ul class="menu w-full grow">
                    <li>
                        <button class="{activeTab === 'home' ? 'active text-accent' : ''} max-lg:is-drawer-close:tooltip max-lg:is-drawer-close:tooltip-right"
                                data-tip="Homepage" onclick={() => navigate('home')}>
                            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5"
                                 stroke="currentColor" class="size-6">
                                <path stroke-linecap="round" stroke-linejoin="round"
                                      d="m2.25 12 8.954-8.955c.44-.439 1.152-.439 1.591 0L21.75 12M4.5 9.75v10.125c0 .621.504 1.125 1.125 1.125H9.75v-4.875c0-.621.504-1.125 1.125-1.125h2.25c.621 0 1.125.504 1.125 1.125V21h4.125c.621 0 1.125-.504 1.125-1.125V9.75M8.25 21h8.25"/>
                            </svg>
                            <span class="max-lg:is-drawer-close:hidden">Home</span>
                        </button>
                    </li>
                    <li>
                        <button class="{activeTab === 'status' ? 'active text-accent' : ''} max-lg:is-drawer-close:tooltip max-lg:is-drawer-close:tooltip-right"
                                data-tip="Status" onclick={() => navigate('status')}>
                            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5"
                                 stroke="currentColor" class="size-6">
                                <path stroke-linecap="round" stroke-linejoin="round"
                                      d="M8.288 15.038a5.25 5.25 0 0 1 7.424 0M5.106 11.856c3.807-3.808 9.98-3.808 13.788 0M1.924 8.674c5.565-5.565 14.587-5.565 20.152 0M12.53 18.22l-.53.53-.53-.53a.75.75 0 0 1 1.06 0Z"/>
                            </svg>
                            <span class="max-lg:is-drawer-close:hidden">Status</span>
                        </button>
                    </li>
                    <li>
                        <button class="{activeTab === 'config' ? 'active text-accent' : ''} max-lg:is-drawer-close:tooltip max-lg:is-drawer-close:tooltip-right" data-tip="Configuration" onclick={() => navigate('config')}>
                            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="size-6">
                                <path stroke-linecap="round" stroke-linejoin="round" d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.241-.438.613-.43.992a7.723 7.723 0 0 1 0 .255c-.008.378.137.75.43.991l1.004.827c.424.35.534.955.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.331.183-.581.495-.644.869l-.213 1.281c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-.255c.007-.38-.138-.751-.43-.992l-1.004-.827a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.183.582-.495.644-.869l.214-1.28Z" />
                                <path stroke-linecap="round" stroke-linejoin="round" d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z" />
                            </svg>
                            <span class="max-lg:is-drawer-close:hidden">Configuration</span>
                        </button>
                    </li>

                    <div class="flex-1"></div>

                    <li class="px-4 py-2 max-lg:is-drawer-close:hidden">

                        <label class="flex cursor-pointer gap-2">
                            <svg
                                    xmlns="http://www.w3.org/2000/svg"
                                    width="20"
                                    height="20"
                                    viewBox="0 0 24 24"
                                    fill="none"
                                    stroke="currentColor"
                                    stroke-width="2"
                                    stroke-linecap="round"
                                    stroke-linejoin="round">
                                <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>
                            </svg>
                            <input type="checkbox" value="light" class="toggle theme-controller"/>
                            <svg
                                    xmlns="http://www.w3.org/2000/svg"
                                    width="20"
                                    height="20"
                                    viewBox="0 0 24 24"
                                    fill="none"
                                    stroke="currentColor"
                                    stroke-width="2"
                                    stroke-linecap="round"
                                    stroke-linejoin="round">
                                <circle cx="12" cy="12" r="5"/>
                                <path
                                        d="M12 1v2M12 21v2M4.2 4.2l1.4 1.4M18.4 18.4l1.4 1.4M1 12h2M21 12h2M4.2 19.8l1.4-1.4M18.4 5.6l1.4-1.4"/>
                            </svg>
                        </label>
                    </li>
                </ul>
            </div>
        </div>
    </div>

</main>