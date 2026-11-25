// The board limits text in bytes of UTF-8, which accented letters take more than one of
export function byteLength(text: string): number {
    return new TextEncoder().encode(text).length;
}

export function formatBytes(bytes: number): string {
    if (bytes >= 1024 * 1024) return `${(bytes / 1024 / 1024).toFixed(2)} MB`;
    return `${(bytes / 1024).toFixed(1)} KB`;
}

// "Paris, Berlin, Rome (CET/CEST)" → name "Paris, Berlin, Rome", detail "CET/CEST"
export function splitLabel(label: string): { name: string, detail: string } {
    const match = /^(.*?)\s*\(([^)]*)\)$/.exec(label);
    return match ? { name: match[1], detail: match[2] } : { name: label, detail: '' };
}
