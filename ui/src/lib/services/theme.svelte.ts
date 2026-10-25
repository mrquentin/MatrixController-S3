export type ThemeMode = 'system' | 'light' | 'dark';

const STORAGE_KEY = 'theme';

class ThemeStore {
    mode = $state<ThemeMode>('system');

    init(): void {
        const saved = localStorage.getItem(STORAGE_KEY);
        if (saved === 'light' || saved === 'dark') this.mode = saved;
        this.apply();
    }

    set(mode: ThemeMode): void {
        this.mode = mode;
        if (mode === 'system') localStorage.removeItem(STORAGE_KEY);
        else localStorage.setItem(STORAGE_KEY, mode);
        this.apply();
    }

    // No data-theme lets daisyUI follow the OS preference (--prefersdark)
    private apply(): void {
        const root = document.documentElement;
        if (this.mode === 'system') root.removeAttribute('data-theme');
        else root.setAttribute('data-theme', this.mode);
    }
}

export const themeStore = new ThemeStore();
