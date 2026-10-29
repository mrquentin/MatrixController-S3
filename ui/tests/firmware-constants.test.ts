import {readFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {describe, expect, it} from 'vitest';
import {DEFAULT_DISPLAY_CONFIG, MAX_BRIGHTNESS, UPDATE_PASSWORD_MAX, UPDATE_PASSWORD_MIN} from '../src/lib/config';

// config.ts mirrors some of the firmware's constants. These read them from the firmware's
// sources, so changing one side without the other fails here instead of on the board

function firmwareSource(path: string): string {
    return readFileSync(fileURLToPath(new URL(`../../${path}`, import.meta.url)), 'utf8');
}

function constant(source: string, pattern: RegExp): string {
    const match = pattern.exec(source);
    if (!match) throw new Error(`${pattern} not found in the firmware source`);
    return match[1];
}

describe('display settings', () => {
    const source = firmwareSource('include/display/DisplayConfig.h');

    it('share the brightness limit', () => {
        expect(MAX_BRIGHTNESS).toBe(Number(constant(source, /MaxBrightness\s*=\s*(\d+)/)));
    });

    it('share the defaults', () => {
        // The default is an enumerator; parseOrientationMode gives the name it has in the config file
        const orientation = constant(source, /OrientationMode orientation\s*=\s*OrientationMode::(\w+)/);
        const orientationName = constant(firmwareSource('src/display/Orientation.cpp'),
            new RegExp(`name == "(\\w+)"\\) return OrientationMode::${orientation};`));
        expect(DEFAULT_DISPLAY_CONFIG).toEqual({
            width: Number(constant(source, /uint16_t width\s*=\s*(\d+)/)),
            height: Number(constant(source, /uint16_t height\s*=\s*(\d+)/)),
            brightness: Number(constant(source, /uint8_t brightness\s*=\s*(\d+)/)),
            doubleBuffer: constant(source, /bool doubleBuffer\s*=\s*(true|false)/) === 'true',
            orientation: orientationName,
        });
    });
});

describe('update password', () => {
    const source = firmwareSource('src/ota/UpdatePassword.cpp');

    it('has the same length limits', () => {
        expect(UPDATE_PASSWORD_MIN).toBe(Number(constant(source, /MinLength\s*=\s*(\d+)/)));
        expect(UPDATE_PASSWORD_MAX).toBe(Number(constant(source, /MaxLength\s*=\s*(\d+)/)));
    });
});
