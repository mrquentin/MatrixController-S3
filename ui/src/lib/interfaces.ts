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
