export const ENDPOINTS = {
    SYSTEM_STATUS:    '/api/system/status',
    SYSTEM_CONFIG:    '/api/system/config',
    WIFI:             '/api/system/wifi',
    WIFI_SCAN:        '/api/system/wifi/scan',
    WIFI_FORGET:      '/api/system/wifi/forget',
    RESTART:          '/api/system/restart',
    UPDATE:           '/api/system/update',
    UPDATE_VERIFY:    '/api/system/update/verify',
    UPDATE_PASSWORD:  '/api/system/update/password',
    RELEASE_CHECK:    '/api/system/update/release/check',
    RELEASE_INSTALL:  '/api/system/update/release/install',
    APPS:             '/api/apps',
    APP_ACTIVE:       '/api/apps/active',
    APP_CONFIG:       '/api/apps/config',
    WEB_SOCKET:       '/ws',
}

// Requests that update the firmware carry the update password in this header
export const UPDATE_PASSWORD_HEADER = 'X-Update-Password';

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

export const ORIENTATION_LABELS = { auto: 'Auto', upright: 'Upright', upsideDown: 'Flipped' } as const;
