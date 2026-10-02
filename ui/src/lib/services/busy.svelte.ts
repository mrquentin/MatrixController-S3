class BusyStore {
    private _count = $state(0);

    get active(): boolean {
        return this._count > 0;
    }

    begin(): void {
        this._count++;
    }

    end(): void {
        if (this._count > 0) this._count--;
    }

    async run<T>(operation: () => Promise<T>): Promise<T> {
        this.begin();
        try {
            return await operation();
        } finally {
            this.end();
        }
    }
}

export const busyStore = new BusyStore();