import {
    createContext,
    useCallback,
    useContext,
    useEffect,
    useMemo,
    useRef,
    useState,
    type ReactNode,
} from "react";

export type ConnectionStatus = "disconnected" | "connecting" | "connected" | "error";

export interface VideoWallContextValue {
    status: ConnectionStatus;
    activeWall: string | null;
    connectionError: string | null;
    lastMessage: unknown;
    connect: (wallName: string) => void;
    disconnect: () => void;
    sendCommand: (commandName: string, args?: unknown[]) => Promise<unknown>;
}

interface PendingResponse {
    resolve: (message: unknown) => void;
    reject: (error: Error) => void;
    timeoutId: ReturnType<typeof setTimeout>;
}

const COMMAND_TIMEOUT_MS = 10_000;
const VideoWallContext = createContext<VideoWallContextValue | null>(null);

function getErrorMessage(error: unknown): string {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
}

export function VideoWallProvider({ children }: { children: ReactNode }) {
    const socketRef = useRef<WebSocket | null>(null);
    const pendingResponseRef = useRef<PendingResponse | null>(null);
    // The server protocol has no request ID, so command/reply exchanges are serialized.
    const commandQueueRef = useRef<Promise<void>>(Promise.resolve());

    const [status, setStatus] = useState<ConnectionStatus>("disconnected");
    const [activeWall, setActiveWall] = useState<string | null>(null);
    const [connectionError, setConnectionError] = useState<string | null>(null);
    const [lastMessage, setLastMessage] = useState<unknown>(null);

    const rejectPendingResponse = useCallback((message: string) => {
        const pending = pendingResponseRef.current;
        if (!pending) return;

        clearTimeout(pending.timeoutId);
        pendingResponseRef.current = null;
        pending.reject(new Error(message));
    }, []);

    const disconnect = useCallback(() => {
        const socket = socketRef.current;
        socketRef.current = null;

        rejectPendingResponse("The WebSocket connection was closed before the command completed.");
        if (socket && socket.readyState < WebSocket.CLOSING) {
            socket.close(1000, "Client disconnected");
        }

        setStatus("disconnected");
        setActiveWall(null);
        setConnectionError(null);
        setLastMessage(null);
    }, [rejectPendingResponse]);

    const connect = useCallback((wallName: string) => {
        const name = wallName.trim();
        if (!name) {
            setStatus("error");
            setConnectionError("Choose a video wall before connecting.");
            return;
        }

        // Tear down any existing socket so this provider always owns one connection.
        const oldSocket = socketRef.current;
        socketRef.current = null;
        rejectPendingResponse("The WebSocket connection changed before the command completed.");
        if (oldSocket && oldSocket.readyState < WebSocket.CLOSING) {
            oldSocket.close(1000, "Switching video walls");
        }

        setStatus("connecting");
        setActiveWall(null);
        setConnectionError(null);
        setLastMessage(null);

        try {
            const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
            const url = `${protocol}//${window.location.host}/api/video-wall/${encodeURIComponent(name)}`;
            const socket = new WebSocket(url);
            socketRef.current = socket;

            socket.addEventListener("open", () => {
                if (socketRef.current !== socket) return;
                setStatus("connected");
                setActiveWall(name);
                setConnectionError(null);
            });

            socket.addEventListener("message", (event: MessageEvent<unknown>) => {
                if (socketRef.current !== socket) return;

                let message: unknown = event.data;
                if (typeof event.data === "string") {
                    try {
                        message = JSON.parse(event.data) as unknown;
                    } catch {
                        // Keep raw text available to the caller, which can report an invalid response.
                    }
                }

                setLastMessage(message);

                const pending = pendingResponseRef.current;
                if (pending) {
                    clearTimeout(pending.timeoutId);
                    pendingResponseRef.current = null;
                    pending.resolve(message);
                }
            });

            socket.addEventListener("error", () => {
                if (socketRef.current !== socket) return;
                setStatus("error");
                setConnectionError("A WebSocket connection error occurred.");
            });

            socket.addEventListener("close", (event) => {
                if (socketRef.current !== socket) return;
                socketRef.current = null;
                rejectPendingResponse("The WebSocket connection closed before the command completed.");
                setActiveWall(null);
                setStatus((currentStatus) => currentStatus === "error" ? "error" : "disconnected");
                if (event.code !== 1000) {
                    setConnectionError((currentError) => currentError ?? "The WebSocket connection was closed unexpectedly.");
                }
            });
        } catch (error) {
            socketRef.current = null;
            setStatus("error");
            setConnectionError(getErrorMessage(error));
        }
    }, [rejectPendingResponse]);

    const sendCommand = useCallback((commandName: string, args: unknown[] = []) => {
        // Queue commands because the current server response format has no correlation ID.
        const task = commandQueueRef.current.catch(() => undefined).then(() => {
            const socket = socketRef.current;
            if (!socket || socket.readyState !== WebSocket.OPEN) {
                throw new Error("Connect to a video wall before sending commands.");
            }

            return new Promise<unknown>((resolve, reject) => {
                const timeoutId = setTimeout(() => {
                    if (pendingResponseRef.current?.timeoutId !== timeoutId) return;
                    pendingResponseRef.current = null;

                    // With no request IDs, keeping this socket open could mistake a late
                    // response for the next command's response. Force a reconnect instead.
                    const timedOutSocket = socketRef.current;
                    socketRef.current = null;
                    if (timedOutSocket && timedOutSocket.readyState < WebSocket.CLOSING) {
                        timedOutSocket.close(4000, "Command response timeout");
                    }
                    const message = `Timed out waiting for a response to '${commandName}'. Reconnect and try again.`;
                    setStatus("error");
                    setActiveWall(null);
                    setConnectionError(message);
                    reject(new Error(message));
                }, COMMAND_TIMEOUT_MS);

                pendingResponseRef.current = { resolve, reject, timeoutId };

                try {
                    socket.send(JSON.stringify({
                        "command-name": commandName,
                        args,
                    }));
                } catch (error) {
                    clearTimeout(timeoutId);
                    pendingResponseRef.current = null;
                    reject(new Error(getErrorMessage(error)));
                }
            });
        });

        commandQueueRef.current = task.then(() => undefined, () => undefined);
        return task;
    }, []);

    useEffect(() => () => {
        const socket = socketRef.current;
        socketRef.current = null;
        rejectPendingResponse("The application was closed before the command completed.");
        if (socket && socket.readyState < WebSocket.CLOSING) {
            socket.close(1000, "Application unmounted");
        }
    }, [rejectPendingResponse]);

    const value = useMemo<VideoWallContextValue>(() => ({
        status,
        activeWall,
        connectionError,
        lastMessage,
        connect,
        disconnect,
        sendCommand,
    }), [status, activeWall, connectionError, lastMessage, connect, disconnect, sendCommand]);

    return (
        <VideoWallContext.Provider value={value}>
            {children}
        </VideoWallContext.Provider>
    );
}

export function useVideoWall(): VideoWallContextValue {
    const context = useContext(VideoWallContext);
    if (!context) {
        throw new Error("useVideoWall must be used inside a VideoWallProvider.");
    }
    return context;
}
