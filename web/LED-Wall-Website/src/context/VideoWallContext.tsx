import {createContext, type ReactNode, useCallback, useContext, useEffect, useMemo, useRef, useState,} from "react";

export type ConnectionStatus = "disconnected" | "connecting" | "connected" | "error";

export type Location = { x: number; y: number; };
export type Size = { width: number; height: number; };
export type RGB = { r: number; g: number; b: number };

type CanvasElementBase = {
    id: string;
    name: string;
    type: string;
    location: Location;
    rotation: number;
    size: Size;
    preserveAspectRatio: boolean;
};

export type ImageCanvasElement = CanvasElementBase & {
    type: "image";
    filepath: string;
};

export type CarouselCanvasElement = CanvasElementBase & {
    type: "carousel";
    filePaths: string[];
    framerate: number;
};

export type VideoCanvasElement = CanvasElementBase & {
    type: "video";
    filepath: string;
    framerate: number;
};

export type RTMPCanvasElement = CanvasElementBase & {
    type: "rtmp";
    streamName: string;
    framerate: number;
};

export type WebBrowserCookie = {
    name: string;
    value: string;
    domain: string;
    path: string;
    secure: boolean;
    httpOnly: boolean;
    sameSite: CookieSameSite
};

export type WebBrowserCanvasElement = CanvasElementBase & {
    type: "web-browser";
    url: string;
    framerate: number;
    viewSize: Size;
    cookies: WebBrowserCookie[];
};

export type TextCanvasElement = CanvasElementBase & {
    type: "text";
    text: string;
    fontPath: string;
    fontSize: number;
    color: RGB;
};

export type CanvasElement =
    | ImageCanvasElement
    | CarouselCanvasElement
    | VideoCanvasElement
    | RTMPCanvasElement
    | WebBrowserCanvasElement
    | TextCanvasElement;

export type MatrixPos = {
    location: Location;
    size: Size;
};

export interface VideoWallContextValue {
    status: ConnectionStatus;
    activeWall: string | null;
    connectionError: string | null;
    lastMessage: unknown;
    connect: (wallName: string) => void;
    disconnect: () => void;
    sendCommand: (commandName: string, args?: unknown[]) => Promise<unknown>;
    activeConfig: string | null;
    setActiveConfig: (configName: string | null) => void;
    autoSave: boolean;
    setAutoSave: (enabled: boolean) => void;
    saveCanvas: (configName?: string) => Promise<unknown>;
    loadCanvas: (configName: string | null) => Promise<void>;
    /** Canvas elements in the server's native bottom-to-top stacking order. */
    canvas: CanvasElement[];
    canvasLoading: boolean;
    canvasError: string | null;
    canvasRevision: number;
    selectedCanvasElementId: string | null;
    selectCanvasElement: (id: string | null) => void;
    refreshCanvas: () => Promise<CanvasElement[]>;
    moveCanvasElement: (id: string, location: Location) => Promise<void>;
    moveCanvasElementZ: (id: string, direction: "up" | "down") => Promise<void>;
    renameCanvasElement: (id: string, newName: string) => Promise<void>;
    transformCanvasElement: (id: string, size: Size, location: Location, degreesCCW: number) => Promise<void>;
    matrices: MatrixPos[];
    canvasSize: Size;
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

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isNumber(value: unknown): value is number {
    return typeof value === "number";
}

function isString(value: unknown): value is string {
    return typeof value === "string";
}

function isLocation(value: unknown): value is Location {
    return isRecord(value) && isNumber(value.x) && isNumber(value.y);
}

function isSize(value: unknown): value is Size {
    return isRecord(value) &&
        isNumber(value.width) &&
        isNumber(value.height);
}

function isRGB(value: unknown): value is RGB {
    return isRecord(value) &&
        isNumber(value.r) &&
        isNumber(value.g) &&
        isNumber(value.b);
}

function isCookieSameSite(value: unknown): value is CookieSameSite {
    return value === "strict" || value === "lax" || value === "none";
}

function isWebBrowserCookie(value: unknown): value is WebBrowserCookie {
    return isRecord(value) &&
        isString(value.name) &&
        isString(value.value) &&
        isString(value.domain) &&
        isString(value.path) &&
        typeof value.secure === "boolean" &&
        typeof value.httpOnly === "boolean" &&
        isCookieSameSite(value.sameSite);
}

function hasCanvasElementBaseFields(value: Record<string, unknown>): boolean {
    return isString(value.id) &&
        isString(value.name) &&
        isString(value.type) &&
        isLocation(value.location) &&
        isNumber(value.rotation) &&
        isSize(value.size) &&
        typeof value.preserveAspectRatio === "boolean";
}

function isCanvasElement(value: unknown): value is CanvasElement {
    if (!isRecord(value) || !hasCanvasElementBaseFields(value)) return false;

    switch (value.type) {
        case "image":
            return isString(value.filepath);

        case "carousel":
            return Array.isArray(value.filePaths) &&
                value.filePaths.every(isString) &&
                isNumber(value.framerate);

        case "video":
            return isString(value.filepath) &&
                isNumber(value.framerate);

        case "rtmp":
            return isString(value.streamName) &&
                isNumber(value.framerate);

        case "web-browser":
            return isString(value.url) &&
                isNumber(value.framerate) &&
                isSize(value.viewSize) &&
                Array.isArray(value.cookies) &&
                value.cookies.every(isWebBrowserCookie);

        case "text":
            return isString(value.text) &&
                isString(value.fontPath) &&
                isNumber(value.fontSize) &&
                isRGB(value.color);

        default:
            return false;
    }
}

function isMatrixPos(value: unknown): value is MatrixPos {
    return isRecord(value) &&
        isLocation(value.location) &&
        isSize(value.size);
}

function getResponseError(response: unknown): string | null {
    if (!isRecord(response)) return "The video wall returned an invalid response.";
    if (response.success === true) return null;
    if (response.success !== false) {
        return "The video wall did not confirm that the command succeeded.";
    }

    if (typeof response.message === "string" && response.message.trim()) return response.message;
    if (typeof response.error === "string" && response.error.trim()) return response.error;

    const details: string[] = [];
    if (response["bad-invocation"] === true) details.push("Invalid command invocation.");
    if (response["no-such-element"] === true) details.push("Element does not exist.");
    return details.length > 0 ? details.join(" ") : "The command failed.";
}


function parseCanvas(response: unknown): CanvasElement[] {
    if (!isRecord(response)) {
        throw new Error("The video wall returned an invalid canvas response.");
    }
    if (response.success === false) {
        throw new Error(getResponseError(response) ?? "Could not retrieve the canvas.");
    }
    if (!Array.isArray(response.canvas)) {
        throw new Error("The response to canvas-get-elements did not contain a canvas array.");
    }

    if (!Array.isArray(response.canvas) || !response.canvas.every(isCanvasElement)) {
        throw new Error("Invalid canvas elements.");
    }

    return response.canvas;
}

function parseMatrices(response: unknown): [Size, MatrixPos[]] {
    if (!isRecord(response)) {
        throw new Error("The video wall returned an invalid canvas region response.");
    }
    if (response.success === false) {
        throw new Error(getResponseError(response) ?? "Could not retrieve the canvas region.");
    }

    if (!isSize(response.size)) {
        throw new Error("The response to canvas-get-region did not contain a valid canvas size.");
    }
    if (!Array.isArray(response.matrices) || !response.matrices.every(isMatrixPos)) {
        throw new Error("The response to canvas-get-region did not contain a valid matrices array.");
    }

    return [response.size, response.matrices];
}

export function VideoWallProvider({children}: { children: ReactNode }) {
    const socketRef = useRef<WebSocket | null>(null);
    const pendingResponseRef = useRef<PendingResponse | null>(null);
    // The server protocol has no request ID, so command/reply exchanges are serialized.
    const commandQueueRef = useRef<Promise<void>>(Promise.resolve());

    const [status, setStatus] = useState<ConnectionStatus>("disconnected");
    const [activeWall, setActiveWall] = useState<string | null>(null);
    const [connectionError, setConnectionError] = useState<string | null>(null);
    const [lastMessage, setLastMessage] = useState<unknown>(null);
    const [activeConfig, setActiveConfig] = useState<string | null>(null);
    const [autoSave, setAutoSave] = useState(true);
    const [canvas, setCanvas] = useState<CanvasElement[]>([]);
    const [canvasLoading, setCanvasLoading] = useState(false);
    const [canvasError, setCanvasError] = useState<string | null>(null);
    const [canvasRevision, setCanvasRevision] = useState(0);
    const [selectedCanvasElementId, setSelectedCanvasElementId] = useState<string | null>(null);
    const [matrices, setMatrices] = useState<MatrixPos[]>([]);
    const [canvasSize, setCanvasSize] = useState<Size>({width: 0, height: 0});

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
        setActiveConfig(null);
        setCanvas([]);
        setSelectedCanvasElementId(null);
        setMatrices([]);
        setCanvasLoading(false);
        setCanvasError(null);
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
        setActiveConfig(null);
        setCanvas([]);
        setSelectedCanvasElementId(null);
        setMatrices([]);
        setCanvasLoading(false);
        setCanvasError(null);

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

                pendingResponseRef.current = {resolve, reject, timeoutId};

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

    const saveCanvas = useCallback((configName?: string) => {
        const name = (configName ?? activeConfig)?.trim();
        if (!name) return Promise.reject(new Error("A configuration name is required."));
        return sendCommand("canvas-save", [name]);
    }, [activeConfig, sendCommand]);

    const refreshCanvas = useCallback(async (): Promise<CanvasElement[]> => {
        setCanvasLoading(true);
        try {
            const response = await sendCommand("canvas-get-elements");
            const nextCanvas = parseCanvas(response);
            // Keep the server's bottom-to-top order in shared state.
            setCanvas(nextCanvas);
            setSelectedCanvasElementId((selectedId) =>
                selectedId && nextCanvas.some((element) => element.id === selectedId) ? selectedId : null
            );
            setCanvasError(null);
            setCanvasRevision((revision) => revision + 1);
            return nextCanvas;
        } catch (error) {
            const message = getErrorMessage(error);
            setCanvasError(message);
            throw error;
        } finally {
            setCanvasLoading(false);
        }
    }, [sendCommand]);

    const refreshMatrices = useCallback(async (): Promise<MatrixPos[]> => {
        try {
            const response = await sendCommand("canvas-get-region");
            const [nextCanvasSize, nextMatrices] = parseMatrices(response);

            setCanvasSize(nextCanvasSize);
            setMatrices(nextMatrices);
            setCanvasError(null);
            return nextMatrices;
        } catch (error) {
            const message = getErrorMessage(error);
            setCanvasError(message);
            throw error;
        }
    }, [sendCommand]);

    const saveCanvasAfterChange = useCallback(async () => {
        if (!autoSave || !activeConfig) return;
        try {
            const response = await saveCanvas();
            const responseError = getResponseError(response);
            if (responseError) throw new Error(responseError);
        } catch (error) {
            const message = `Canvas updated, but auto-save failed: ${getErrorMessage(error)}`;
            setCanvasError(message);
            throw new Error(message);
        }
    }, [activeConfig, autoSave, saveCanvas]);

    const loadCanvas = useCallback(async (configName: string | null) => {
        const response = configName
            ? await sendCommand("canvas-load", [configName])
            : await sendCommand("canvas-clear");
        const responseError = getResponseError(response);
        if (responseError) {
            throw new Error(responseError);
        }

        setActiveConfig(configName);
        setSelectedCanvasElementId(null);
        await refreshCanvas();
    }, [refreshCanvas, sendCommand]);

    const moveCanvasElement = useCallback(async (
        id: string,
        location: Location
    ) => {
        try {
            const response = await sendCommand(`element-move`, [id, String(Math.trunc(location.x)), String(Math.trunc(location.y))]);
            const responseError = getResponseError(response);
            if (responseError) throw new Error(responseError);
            await saveCanvasAfterChange();
        } catch (error) {
            setCanvasError(getErrorMessage(error));
            throw error;
        }
    }, [refreshCanvas, saveCanvasAfterChange, sendCommand]);

    const moveCanvasElementZ = useCallback(async (
        id: string,
        direction: "up" | "down",
    ) => {
        try {
            const response = await sendCommand(`element-move-${direction}`, [id]);
            const responseError = getResponseError(response);
            if (responseError) throw new Error(responseError);
            await refreshCanvas();
            await saveCanvasAfterChange();
        } catch (error) {
            setCanvasError(getErrorMessage(error));
            throw error;
        }
    }, [refreshCanvas, saveCanvasAfterChange, sendCommand]);

    const renameCanvasElement = useCallback(async (id: string, newName: string) => {
        const trimmedName = newName.trim();
        if (!trimmedName) throw new Error("An element name cannot be empty.");

        try {
            const response = await sendCommand("element-rename", [id, trimmedName]);
            const responseError = getResponseError(response);
            if (responseError) throw new Error(responseError);
            await refreshCanvas();
            await saveCanvasAfterChange();
        } catch (error) {
            setCanvasError(getErrorMessage(error));
            throw error;
        }
    }, [refreshCanvas, saveCanvasAfterChange, sendCommand]);


    const transformCanvasElement = useCallback(async (
        id: string,
        size: Size,
        location: Location,
        degreesCCW: number,
    ) => {
        const element = canvas.find((candidate) => candidate.id === id);
        if (!element) throw new Error("The selected canvas element no longer exists.");

        const width = Math.max(1, Math.round(size.width));
        const height = Math.max(1, Math.round(size.height));
        const x = Math.round(location.x);
        const y = Math.round(location.y);
        const rotation = Number(degreesCCW.toFixed(3));

        try {
            if (width !== Math.round(element.size.width) || height !== Math.round(element.size.height)) {
                const response = await sendCommand("element-resize", [id, String(width), String(height)]);
                const responseError = getResponseError(response);
                if (responseError) throw new Error(responseError);
            }

            if (rotation !== 0) {
                const response = await sendCommand("element-rotate", [id, String(rotation)]);
                const responseError = getResponseError(response);
                if (responseError) throw new Error(responseError);
            }

            if (x !== Math.round(element.location.x) || y !== Math.round(element.location.y)) {
                const response = await sendCommand("element-move", [id, String(x), String(y)]);
                const responseError = getResponseError(response);
                if (responseError) throw new Error(responseError);
            }

            await saveCanvasAfterChange();
        } catch (error) {
            setCanvasError(getErrorMessage(error));
            throw error;
        }
    }, [canvas, saveCanvasAfterChange, sendCommand]);

    useEffect(() => {
        if (status === "connected") {
            void refreshMatrices().catch(() => undefined);
            void refreshCanvas().catch(() => undefined);
        } else {
            setCanvas([]);
            setSelectedCanvasElementId(null);
            setMatrices([]);
            setCanvasLoading(false);
            setCanvasError(null);
        }
    }, [status, refreshCanvas]);

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
        activeConfig,
        setActiveConfig,
        autoSave,
        setAutoSave,
        saveCanvas,
        loadCanvas,
        canvas,
        canvasLoading,
        canvasError,
        canvasRevision,
        selectedCanvasElementId,
        selectCanvasElement: setSelectedCanvasElementId,
        refreshCanvas,
        moveCanvasElement,
        moveCanvasElementZ,
        renameCanvasElement,
        transformCanvasElement,
        matrices,
        canvasSize,
    }), [
        status,
        activeWall,
        connectionError,
        lastMessage,
        connect,
        disconnect,
        sendCommand,
        activeConfig,
        autoSave,
        saveCanvas,
        loadCanvas,
        canvas,
        canvasLoading,
        canvasError,
        canvasRevision,
        selectedCanvasElementId,
        setSelectedCanvasElementId,
        refreshCanvas,
        moveCanvasElement,
        moveCanvasElementZ,
        renameCanvasElement,
        transformCanvasElement,
        matrices,
        canvasSize,
    ]);

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
