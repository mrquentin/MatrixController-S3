// An open WebSocket to the board, so the UI can show whether it's reachable. The board sends nothing on it.
const RECONNECT_MS = 3000;

export class SocketService {
    public isConnected = $state<boolean>(false);

    private socket: WebSocket | null = null;

    connect(url: string) {
        if (this.socket) return;

        this.socket = new WebSocket(url);
        this.socket.onopen = () => this.isConnected = true;
        // After a restart the board is back on the same address
        this.socket.onclose = () => {
            this.isConnected = false;
            this.socket = null;
            setTimeout(() => this.connect(url), RECONNECT_MS);
        };
    }
}

export const socketService = new SocketService();
