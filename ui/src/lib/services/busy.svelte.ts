class BusyStore {
    private _count = $state(0);

    get active(): boolean {
        return this._count > 0;
    }

    async run<T>(operation: () => Promise<T>): Promise<T> {
        this._count++;
        try {
            return await operation();
        } finally {
            this._count--;
        }
    }
}

export const busyStore = new BusyStore();