export interface SystemStatus {
    cpuFreqMHz: number;
    chipModel: string;
    memoryFreeHeap: number;
    memoryMaxBlock: number;
    memoryMinFreeHeap: number;
    psramFree: number;
    psramSize: number;
    wifiChannel: number;
    wifiConnected: boolean;
    wifiGatewayIp: string;
    wifiIp: string;
    wifiMACAddress: string;
    wifiMDNSName: string;
    wifiQuality: number;
    wifiRssi: number;
    uptimeSeconds: number;
    resetReason: string;
    firmwareVersion: string;
}

export interface DisplayConfig {
    width: number;
    height: number;
    brightness: number;
    doubleBuffer: boolean;
}

export interface SystemConfig {
    debug: boolean;
    display: DisplayConfig;
    // POSIX TZ string, see TIMEZONES in config.ts
    timezone: string;
}

// GET /api/system/update, see FirmwareUpdate::describe on the board
export interface UpdateStatus {
    version: string;
    // App slot running now, "ota_0" or "ota_1"
    slot: string;
    // Freshly updated and not accepted yet: a crash or restart now goes back to the previous version
    onProbation: boolean;
    // The last update didn't start properly and the board went back to this version
    rolledBack: boolean;
    state: 'idle' | 'running' | 'installed' | 'failed';
    source: 'upload' | 'network' | 'release' | null;
    // The update replaces the file system rather than the firmware (PlatformIO uploadfs)
    files: boolean;
    written: number;
    // 0 when unknown
    size: number;
    error?: string;
    passwordSet: boolean;
    networkUpload: { listening: boolean, port: number, restartNeeded: boolean };
    // null when the firmware was built without a release repository
    release: ReleaseStatus | null;
}

export interface ReleaseStatus {
    repo: string;
    busy: boolean;
    job: 'check' | 'install';
    // Present once a check found a release
    latest?: string;
    publishedAt?: string;
    size?: number;
    updateAvailable?: boolean;
    error?: string;
}

export interface WifiNetwork {
    ssid: string;
    // 0-100, from the signal strength
    quality: number;
    secure: boolean;
}

// One setting of an app, as described by the firmware's ConfigSchema
export type AppField = { key: string; label: string } & (
    | { type: 'text'; maxLength: number }
    | { type: 'number'; min: number; max: number }
    | { type: 'bool' }
    | { type: 'color' }
    | { type: 'select'; options: { value: string; label: string }[] }
);

export type AppConfig = Record<string, string | number | boolean>;

export interface AppInfo {
    id: string;
    name: string;
    schema: AppField[];
    config: AppConfig;
}

export interface AppsState {
    active: string | null;
    apps: AppInfo[];
}
