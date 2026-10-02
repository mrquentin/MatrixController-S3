const IS_DEV = import.meta.env.DEV;

export const BASE_URL = IS_DEV ? '' : '';

export const ENDPOINTS = {
    GET_SYSTEM_STATUS:  `${BASE_URL}/api/system/status`,
    GET_SYSTEM_CONFIG: `${BASE_URL}/api/system/config`,
    POST_SYSTEM_CONFIG: `${BASE_URL}/api/system/config`,
    WEB_SOCKET:         `${BASE_URL}/ws`
}