import {afterEach, describe, expect, it, vi} from 'vitest';
import {apiRequest, errorDetails} from '../src/lib/api';

function respond(status: number, body: unknown) {
    const text = typeof body === 'string' ? body : JSON.stringify(body);
    return vi.fn().mockResolvedValue(new Response(text, { status }));
}

afterEach(() => {
    vi.unstubAllGlobals();
});

describe('errorDetails', () => {
    it('uses the details of a board error', () => {
        expect(errorDetails('{"result":"error","details":"Wrong update password"}', 401)).toBe('Wrong update password');
    });

    it('falls back to the status for anything else', () => {
        expect(errorDetails('Not found', 404)).toBe('Server Error: 404');
        expect(errorDetails('{"result":"error"}', 500)).toBe('Server Error: 500');
        expect(errorDetails('', 503)).toBe('Server Error: 503');
    });
});

describe('apiRequest', () => {
    it('returns the parsed answer', async () => {
        vi.stubGlobal('fetch', respond(200, { ssid: 'Home' }));
        expect(await apiRequest<{ ssid: string }>('/api/system/wifi')).toEqual({
            data: { ssid: 'Home' }, error: null, loading: false
        });
    });

    it('sends JSON bodies and extra headers', async () => {
        const fetch = respond(200, { result: 'success' });
        vi.stubGlobal('fetch', fetch);
        await apiRequest('/api/system/update/verify', 'POST', { a: 1 }, { 'X-Update-Password': 'secret' });

        const [url, options] = fetch.mock.calls[0];
        expect(url).toBe('/api/system/update/verify');
        expect(options.method).toBe('POST');
        expect(options.body).toBe('{"a":1}');
        expect(options.headers).toMatchObject({ 'Content-Type': 'application/json', 'X-Update-Password': 'secret' });
    });

    it('reports the board\'s error details', async () => {
        vi.stubGlobal('fetch', respond(429, { result: 'error', details: 'Too many wrong passwords' }));
        expect(await apiRequest('/api/system/update/verify', 'POST')).toEqual({
            data: null, error: 'Too many wrong passwords', loading: false
        });
    });

    it('reports network failures', async () => {
        vi.stubGlobal('fetch', vi.fn().mockRejectedValue(new TypeError('Failed to fetch')));
        expect((await apiRequest('/api/system/status')).error).toBe('Failed to fetch');
    });
});
