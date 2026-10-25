import type {DisplayConfig} from "$lib/interfaces";

const IS_DEV = import.meta.env.DEV;

export const BASE_URL = IS_DEV ? '' : '';

export const ENDPOINTS = {
    GET_SYSTEM_STATUS:  `${BASE_URL}/api/system/status`,
    GET_SYSTEM_CONFIG: `${BASE_URL}/api/system/config`,
    POST_SYSTEM_CONFIG: `${BASE_URL}/api/system/config`,
    WEB_SOCKET:         `${BASE_URL}/ws`
}

// Mirrors the firmware defaults in include/display/DisplayConfig.h
export const DEFAULT_DISPLAY_CONFIG: DisplayConfig = {
    width: 128,
    height: 64,
    brightness: 128,
    doubleBuffer: true
}

// Mirrors MaxBrightness in include/display/DisplayConfig.h
export const MAX_BRIGHTNESS = 128;

export const DISPLAY_HEIGHTS = [16, 32, 64];
