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
}

export interface SystemConfig {
    debug: boolean;
}