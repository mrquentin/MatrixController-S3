export interface SystemStatus {
    cpuFreqMHz: number;
    chipModel: string;
    memoryFreeHeap: number;
    memoryMaxBlock: number;
    memoryMinFreeHeap: number;
    psramFree: number;
    psramSize: number;
    wifiConnected: boolean;
    // The rest of wifi* only while connected
    wifiChannel?: number;
    wifiGatewayIp?: string;
    wifiIp?: string;
    wifiMACAddress?: string;
    wifiMDNSName?: string;
    wifiQuality?: number;
    wifiRssi?: number;
    uptimeSeconds: number;
    // Why the chip last restarted, e.g. "POWER_ON", "BROWNOUT"
    resetReason: string;
    // A restart nobody asked for: crash, hang or power dip
    resetFault: boolean;
    firmwareVersion: string;
    orientation: OrientationStatus;
}

// Which way up the panel is shown, and the accelerometer reading behind it
export interface OrientationStatus {
    // Whether the board's accelerometer answered at boot
    sensor: boolean;
    mode: OrientationMode;
    upsideDown: boolean;
    // Latest reading in g, only with a sensor. The axis pointing up reads about +1
    x?: number;
    y?: number;
    z?: number;
}

// Landscape only: auto follows the accelerometer, the others keep one way up
export type OrientationMode = 'auto' | 'upright' | 'upsideDown';

export interface DisplayConfig {
    brightness: number;
    doubleBuffer: boolean;
    orientation: OrientationMode;
}

export interface SystemConfig {
    debug: boolean;
    display: DisplayConfig;
    // POSIX TZ string, see TIMEZONES in config.ts
    timezone: string;
}

// The panel's fixed size, and what the board accepts for the display settings
export interface ConfigLimits {
    panelWidth: number;
    panelHeight: number;
    maxBrightness: number;
}

// GET /api/system/config, see Config::describe on the board
export interface ConfigState {
    config: SystemConfig;
    limits: ConfigLimits;
}

// GET /api/system/update, see FirmwareUpdate::describe on the board
export interface UpdateStatus {
    version: string;
    // App slot running now, "ota_0" or "ota_1"
    slot: string;
    // Freshly updated and not accepted yet: a crash or restart now goes back to the previous version
    onProbation: boolean;
    // How long a fresh update stays on probation
    probationSeconds: number;
    // The last update didn't start properly and the board went back to this version
    rolledBack: boolean;
    state: 'idle' | 'running' | 'installed' | 'failed';
    source: 'upload' | 'network' | 'release' | null;
    // The update replaces the file system rather than the firmware (PlatformIO uploadfs)
    files: boolean;
    written: number;
    // 0 when unknown
    size: number;
    // Largest image that fits the app slot
    maxSize: number;
    error?: string;
    passwordSet: boolean;
    passwordMinLength: number;
    passwordMaxLength: number;
    networkUpload: { listening: boolean, port: number, restartNeeded: boolean };
    // null when the firmware was built without a release repository
    release: ReleaseStatus | null;
}

export interface ReleaseStatus {
    repo: string;
    // How often the board looks for a new release by itself
    checkIntervalHours: number;
    busy: boolean;
    job: 'check' | 'install';
    // Present once a check found a release
    latest?: string;
    publishedAt?: string;
    // Newer than the running firmware
    updateAvailable?: boolean;
    checkedSecondsAgo?: number;
    error?: string;
}

// GET /api/system/wifi
export interface WifiState {
    ssid: string;
    // The address the board answers to on its network, e.g. "matrixcontroller.local"
    hostname: string;
}

export interface WifiNetwork {
    ssid: string;
    // 0-100, from the signal strength
    quality: number;
    secure: boolean;
}

// One setting of an app, as described by the firmware's AppSettings
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
