import {useEffect, useState} from "react";
import {useVideoWall} from "../context/VideoWallContext.tsx";
import {Check, MoreHorizontal} from "lucide-react";

interface ConfigFile {
    value: string;
    label: string;
}

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function displayConfigName(path: string): string {
    const filename = path.split(/[\\/]/).pop() ?? path;
    return filename.replace(".yaml", "");
}

function parseConfigFiles(data: unknown): ConfigFile[] {
    if (isRecord(data) && data.success === false) {
        const message = typeof data.message === "string" ? data.message : "The server could not list configuration files.";
        throw new Error(message);
    }

    let entries: unknown;
    if (Array.isArray(data)) {
        entries = data;
    } else if (isRecord(data)) {
        entries = data.files ?? data.configs ?? data["config-files"] ?? data.config_files ?? data.configFiles ?? data.configurations;
    }

    if (!Array.isArray(entries)) {
        throw new Error("The response from /api/list-config-files did not contain a file list.");
    }

    const configs = entries.flatMap((entry): ConfigFile[] => {
        if (typeof entry === "string" && entry.trim()) {
            return [{value: entry, label: displayConfigName(entry)}];
        }
        if (!isRecord(entry)) return [];

        const value = [entry.path, entry.file, entry.filename, entry.name]
            .find((candidate): candidate is string => typeof candidate === "string" && candidate.trim().length > 0);
        if (!value) return [];

        const label = [entry.label, entry.displayName, entry.display_name, entry.name]
            .find((candidate): candidate is string => typeof candidate === "string" && candidate.trim().length > 0);
        return [{value, label: label ?? displayConfigName(value)}];
    });

    return configs.filter((config, index) => (
        configs.findIndex((candidate) => candidate.value === config.value) === index
    ));
}

function getResponseError(response: unknown): string | null {
    if (!isRecord(response)) return "The server returned an invalid save response.";
    if (response.success === true) return null;
    if (typeof response.message === "string" && response.message.trim()) return response.message;
    if (typeof response.error === "string" && response.error.trim()) return response.error;
    if (response["bad-invocation"] === true) return "The save command was invalid.";
    return "The server did not confirm that the configuration was saved.";
}

export default function Header() {
    const [walls, setWalls] = useState<string[]>([]);
    const [selectedWall, setSelectedWall] = useState("");
    const [wallListError, setWallListError] = useState<string | null>(null);
    const [configFiles, setConfigFiles] = useState<ConfigFile[]>([]);
    const [configListError, setConfigListError] = useState<string | null>(null);
    const [menuOpen, setMenuOpen] = useState(false);
    const [saving, setSaving] = useState(false);
    const [feedback, setFeedback] = useState<{kind: "success" | "error"; text: string} | null>(null);

    const {
        status,
        activeWall,
        connectionError,
        connect,
        disconnect,
        activeConfig,
        setActiveConfig,
        autoSave,
        setAutoSave,
        saveCanvas,
        loadCanvas,
    } = useVideoWall();

    useEffect(() => {
        const controller = new AbortController();

        async function loadWalls() {
            try {
                const response = await fetch("/api/list-video-walls", {signal: controller.signal});
                if (!response.ok) throw new Error(`Could not load video walls (${response.status}).`);
                const data: unknown = await response.json();
                if (!Array.isArray(data) || !data.every((wall): wall is string => typeof wall === "string")) {
                    throw new Error("The server returned an invalid video wall list.");
                }
                setWalls(data);
                setWallListError(null);
            } catch (error) {
                if (error instanceof Error && error.name !== "AbortError") setWallListError(error.message);
            }
        }

        async function loadConfigFiles() {
            try {
                const response = await fetch("/api/list-config-files", {signal: controller.signal});
                if (!response.ok) throw new Error(`Could not load configuration files (${response.status}).`);
                const data: unknown = await response.json();
                const files = parseConfigFiles(data);
                setConfigFiles(files);
                setConfigListError(null);
            } catch (error) {
                if (error instanceof Error && error.name !== "AbortError") setConfigListError(error.message);
            }
        }

        void loadWalls();
        void loadConfigFiles();
        return () => controller.abort();
    }, []);

    useEffect(() => {
        if (activeWall) setSelectedWall(activeWall);
    }, [activeWall]);

    function handleWallChange(wallName: string) {
        setSelectedWall(wallName);
        setFeedback(null);
        if (wallName) connect(wallName);
        else disconnect();
    }

    function handleDisconnectAction() {
        disconnect();
        setMenuOpen(false);
    }

    function handleConnectAction() {
        connect(selectedWall);
        setMenuOpen(false);
    }

    async function handleConfigChange(configName: string) {
        if (status !== "connected") return;
        setFeedback(null);
        try {
            await loadCanvas(configName || null);
        } catch (error) {
            setFeedback({kind: "error", text: error instanceof Error ? error.message : "Unable to change canvas configuration."});
        }
    }

    async function handleSave() {
        if (status !== "connected" || saving) return;
        let configName = activeConfig;
        if (configName === null) {
            configName = window.prompt("Enter a name for this configuration:");
            if (configName === null) return;
            configName += ".yaml";
        }
        const trimmedName = configName.trim();
        if (!trimmedName) {
            setFeedback({kind: "error", text: "A configuration name is required."});
            return;
        }
        setSaving(true);
        setFeedback(null);
        try {
            const response = await saveCanvas(trimmedName);
            const error = getResponseError(response);
            if (error) throw new Error(error);
            setActiveConfig(trimmedName);
            setFeedback({kind: "success", text: "Saved"});
        } catch (error) {
            setFeedback({
                kind: "error",
                text: error instanceof Error ? error.message : "Unable to save the configuration.",
            });
        } finally {
            setSaving(false);
        }
    }

    const statusLabel = status === "connected"
        ? "ACTIVE"
        : status === "connecting"
            ? "CONNECTING"
            : status === "error"
                ? "ERROR"
                : "DISCONNECTED";
    const headerError = connectionError ?? wallListError ?? configListError;

    return (
        <div className="header-layout">
            <div className="header-group header-wall-group">
                <span className="header-label">Video Wall</span>
                <select
                    className="header-select wall-select"
                    aria-label="Video wall"
                    value={selectedWall}
                    onChange={(event) => handleWallChange(event.target.value)}
                    disabled={walls.length === 0}
                >
                    <option value="">Choose wall</option>
                    {walls.map((wall) => <option key={wall} value={wall}>{wall}</option>)}
                </select>
                <span className={`connection-status status-${status}`} aria-live="polite" title={connectionError ?? undefined}>
                    {statusLabel}
                </span>
                <div className="header-menu-wrap">
                    <button
                        className="header-icon-button more-button"
                        type="button"
                        aria-label="Video wall actions"
                        aria-expanded={menuOpen}
                        title="Video wall actions"
                        onClick={() => setMenuOpen((open) => !open)}
                    >
                        <MoreHorizontal size={21}/>
                    </button>
                    {menuOpen && (
                        <div className="header-menu" role="menu">
                            <button type="button" role="menuitem" onClick={handleConnectAction} disabled={status !== "disconnected" || selectedWall === ""}>
                                Connect
                            </button>
                            <button type="button" role="menuitem" onClick={handleDisconnectAction} disabled={status !== "connected" && status !== "connecting"}>
                                Disconnect
                            </button>
                        </div>
                    )}
                </div>
            </div>

            <div className="header-group header-config-group">
                <span className="header-label">Configuration</span>
                <select
                    className="header-select config-select"
                    aria-label="Configuration"
                    value={activeConfig ?? ""}
                    onChange={(event) => void handleConfigChange(event.target.value)}
                    disabled={status !== "connected"}
                    title={configListError ?? undefined}
                >
                    <option value="">No configuration</option>
                    {configFiles.map((config) => (
                            <option key={config.value} value={config.value}>{config.label}</option>
                        ))}
                </select>
                <button
                    className="header-save-button"
                    type="button"
                    onClick={() => void handleSave()}
                    disabled={status !== "connected" || saving}
                >
                    {saving ? "Saving…" : "Save"}
                </button>
                <label className="autosave-control" title="Save the selected configuration after canvas changes">
                    <input
                        type="checkbox"
                        checked={autoSave}
                        disabled={status !== "connected" || !activeConfig}
                        onChange={(event) => setAutoSave(event.target.checked)}
                    />
                    <span className="autosave-check" aria-hidden="true">
                        {autoSave && <Check size={23} strokeWidth={3}/>}
                    </span>
                    <span>Auto Save</span>
                </label>
                {feedback && (
                    <span className={`save-feedback save-feedback-${feedback.kind}`} role={feedback.kind === "error" ? "alert" : "status"}>
                        {feedback.text}
                    </span>
                )}
                {headerError && !feedback && (
                    <span className="header-error" role="alert" title={headerError}>
                        {headerError}
                    </span>
                )}
            </div>
        </div>
    );
}
