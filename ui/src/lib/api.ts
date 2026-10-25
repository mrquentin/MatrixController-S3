export type ApiResponse<T> = {
    data: T | null;
    error: string | null;
    loading: boolean;
}

// Error answers from the board look like {"result": "error", "details": "..."}
export function errorDetails(body: string, status: number): string {
    try {
        const details = JSON.parse(body)?.details;
        if (typeof details === 'string' && details) return details;
    } catch {
        // Not JSON
    }
    return `Server Error: ${status}`;
}

export async function apiRequest<T>(
    url: string,
    method: 'GET' | 'POST' | 'PUT' = 'GET',
    body?: any,
    headers?: Record<string, string>
): Promise<ApiResponse<T>> {
    try {
        const options: RequestInit = {
            method,
            headers: { 'Content-Type': 'application/json', ...headers },
        }

        if (body) options.body = JSON.stringify(body);

        const response = await fetch(url, options);
        if (!response.ok) {
            return {data: null, error: errorDetails(await response.text(), response.status), loading: false};
        }

        const data = await response.json();
        return {data, error: null, loading: false};
    } catch (err) {
        return {
            data: null,
            error: err instanceof Error ? err.message : "Network unreachable",
            loading: false
        };
    }
}
