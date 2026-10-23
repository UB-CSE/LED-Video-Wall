import { useEffect, useState } from "react";
import { useVideoWall } from "../context/VideoWallContext.tsx";

export default function Header() {
    const [walls, setWalls] = useState<string[]>([]);
    const [selectedWall, setSelectedWall] = useState("");
    const [wallListError, setWallListError] = useState<string | null>(null);
    const { status, activeWall, connectionError, connect, disconnect } = useVideoWall();

    useEffect(() => {
        const controller = new AbortController();

        async function loadWalls() {
            try {
                const response = await fetch("/api/list-video-walls", {
                    signal: controller.signal,
                });

                if (!response.ok) {
                    throw new Error(`Could not load video walls (${response.status}).`);
                }

                const data: unknown = await response.json();
                if (!Array.isArray(data) || !data.every((wall): wall is string => typeof wall === "string")) {
                    throw new Error("The server returned an invalid video wall list.");
                }

                setWalls(data);
                setWallListError(null);
            } catch (error) {
                if (error instanceof Error && error.name !== "AbortError") {
                    setWallListError(error.message);
                }
            }
        }

        void loadWalls();
        return () => controller.abort();
    }, []);

    const statusLabel = status === "connected"
        ? `CONNECTED${activeWall ? ` — ${activeWall}` : ""}`
        : status.toUpperCase();

    function handleConnectClick() {
        if (status === "connected" || status === "connecting") {
            disconnect();
        } else if (selectedWall) {
            connect(selectedWall);
        }
    }

    return (
        <div className="header-controls">
            <span className="header-title">LED Video Wall</span>
            <span className="header-separator" aria-hidden="true" />

            <label className="wall-select-label" htmlFor="wall-select">Video wall</label>
            <select
                id="wall-select"
                value={selectedWall}
                disabled={status === "connected" || status === "connecting"}
                onChange={(event) => setSelectedWall(event.target.value)}
            >
                <option value="">-- Choose a wall --</option>
                {walls.map((wall) => (
                    <option key={wall} value={wall}>{wall}</option>
                ))}
            </select>
            <button
                type="button"
                onClick={handleConnectClick}
                disabled={status !== "connected" && status !== "connecting" && !selectedWall}
            >
                {status === "connected" ? "Disconnect" : status === "connecting" ? "Cancel" : "Connect"}
            </button>

            <span className={`status status-${status}`} aria-live="polite" title={connectionError ?? undefined}>
                {statusLabel}
            </span>
            {(connectionError || wallListError) && (
                <span className="header-error" role="alert">
                    {connectionError ?? wallListError}
                </span>
            )}
        </div>
    );
}
