<script lang="ts">
    // Alert-style confirmation: the action only runs once the user accepts it
    let { open = $bindable(false), title, message, confirmLabel, destructive = false, onConfirm }: {
        open?: boolean,
        title: string,
        message: string,
        confirmLabel: string,
        destructive?: boolean,
        onConfirm: () => void
    } = $props();

    let dialog: HTMLDialogElement;

    $effect(() => {
        if (open && !dialog.open) dialog.showModal();
        else if (!open && dialog.open) dialog.close();
    });

    function confirm() {
        open = false;
        onConfirm();
    }
</script>

<!-- Escape and the backdrop both cancel -->
<dialog bind:this={dialog} class="modal modal-bottom sm:modal-middle" onclose={() => open = false}>
    <!-- The hairline keeps the box apart from a black page in dark mode -->
    <div class="modal-box max-w-sm rounded-box border border-base-content/10 bg-base-100 p-0 text-center">
        <div class="px-5 pt-5 pb-4">
            <h3 class="text-[1.0625rem] font-semibold">{title}</h3>
            <p class="mt-1 text-[0.875rem] leading-snug text-base-content/70">{message}</p>
        </div>
        <div class="grid grid-cols-2 border-t border-base-content/10">
            <button type="button" class="pressable h-11 border-r border-base-content/10 text-primary" onclick={() => open = false}>
                Cancel
            </button>
            <button type="button" class="pressable h-11 font-semibold {destructive ? 'text-error' : 'text-primary'}" onclick={confirm}>
                {confirmLabel}
            </button>
        </div>
    </div>
    <form method="dialog" class="modal-backdrop"><button>Cancel</button></form>
</dialog>
