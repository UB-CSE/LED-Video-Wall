import {type SubmitEvent, useState} from "react";
import {useVideoWall} from "../context/VideoWallContext.tsx";

interface CommandResult {
    state: "idle" | "sending" | "success" | "error";
    message?: string;
}

function getResponseError(response: unknown): string | null {
    if (typeof response !== "object" || response === null || Array.isArray(response)) {
        return "The video wall returned an invalid response.";
    }

    const result = response as Record<string, unknown>;
    if (result.success === true) return null;

    if (typeof result.message === "string" && result.message.trim()) {
        return result.message;
    }
    if (typeof result.error === "string" && result.error.trim()) {
        return result.error;
    }

    const details: string[] = [];
    if (result["bad-invocation"] === true) details.push("Invalid command invocation.");
    if (result["no-such-element"] === true) details.push("Element does not exist.");

    if (details.length > 0) return `Command failed. ${details.join(" ")}`;
    return "The command failed (the response did not report success).";
}

export default function Properties() {
    const {status, sendCommand} = useVideoWall();
    const [configPath, setConfigPath] = useState<string>("");
    const [loadConfigResult, setLoadConfigResult] = useState<CommandResult>({state: "idle"})

    async function submitCommand(event: SubmitEvent<HTMLFormElement>, commandName: string, commandArgs: string[]) {
        event.preventDefault();

        setLoadConfigResult({state: "sending"});
        try {
            const response = await sendCommand(commandName, commandArgs);
            const error = getResponseError(response);
            setLoadConfigResult(
                error
                    ? {state: "error", message: error}
                    : {state: "success", message: "Command completed successfully."},
            );
        } catch (error) {
            setLoadConfigResult({
                state: "error",
                message: error instanceof Error ? error.message : "Unable to send the command.",
            });
        }
    }

    return (
        <div>
            <h3> Load Canvas</h3>
            <form onSubmit={(event) => void submitCommand(
                event,
                "canva-load", new Array<string>(configPath)
            )}>
                <label key="Configuration File">
                    <span>Configuration File</span>
                    <input
                        type="text"
                        value={configPath}
                        placeholder="Config File Path"
                        required={true}
                        onChange={(event) => setConfigPath(event.target.value)}
                    />
                </label>
                <button
                    type="submit"
                    disabled={status !== "connected" || loadConfigResult.state === "sending"}
                >
                    {loadConfigResult.state === "sending" ? "Sending…" : "Submit"}
                </button>
                {loadConfigResult.state === "error" && (
                    <p role="alert">
                        {loadConfigResult.message}
                    </p>
                )}
                {loadConfigResult.state === "success" && (
                    <p role="status">
                        {loadConfigResult.message}
                    </p>
                )}
            </form>
            {status !== "connected" && (
                <p><i>Not connected.</i></p>
            )}
        </div>
    );
}
