import type {DisplayConfig} from "$lib/interfaces";

const IS_DEV = import.meta.env.DEV;

export const BASE_URL = IS_DEV ? '' : '';

export const ENDPOINTS = {
    GET_SYSTEM_STATUS:  `${BASE_URL}/api/system/status`,
    GET_SYSTEM_CONFIG: `${BASE_URL}/api/system/config`,
    POST_SYSTEM_CONFIG: `${BASE_URL}/api/system/config`,
    GET_APPS:           `${BASE_URL}/api/apps`,
    POST_APP_ACTIVE:    `${BASE_URL}/api/apps/active`,
    POST_APP_CONFIG:    `${BASE_URL}/api/apps/config`,
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

export const DEFAULT_TIMEZONE = 'UTC0';

// The board needs POSIX TZ strings, which include the daylight saving rules
export const TIMEZONES = [
    {label: 'UTC', value: 'UTC0'},
    {label: 'London (GMT/BST)', value: 'GMT0BST,M3.5.0/1,M10.5.0'},
    {label: 'Paris, Berlin, Rome (CET/CEST)', value: 'CET-1CEST,M3.5.0,M10.5.0/3'},
    {label: 'Helsinki, Athens (EET/EEST)', value: 'EET-2EEST,M3.5.0/3,M10.5.0/4'},
    {label: 'New York (EST/EDT)', value: 'EST5EDT,M3.2.0,M11.1.0'},
    {label: 'Chicago (CST/CDT)', value: 'CST6CDT,M3.2.0,M11.1.0'},
    {label: 'Denver (MST/MDT)', value: 'MST7MDT,M3.2.0,M11.1.0'},
    {label: 'Los Angeles (PST/PDT)', value: 'PST8PDT,M3.2.0,M11.1.0'},
    {label: 'Tokyo (JST)', value: 'JST-9'},
    {label: 'Sydney (AEST/AEDT)', value: 'AEST-10AEDT,M10.1.0,M4.1.0/3'},
];
