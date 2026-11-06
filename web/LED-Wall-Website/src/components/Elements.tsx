import {type KeyboardEvent, useState} from "react";
import {type CanvasElement, useVideoWall} from "../context/VideoWallContext.tsx";
import {
    Box,
    Check,
    ChevronDown,
    ChevronUp,
    Copy,
    Globe,
    Image as ImageIcon,
    Images,
    type LucideIcon as LucideIconType,
    Pencil,
    Plus,
    Radio,
    Trash2,
    Type,
    Video,
    X,
} from "lucide-react";

interface RenameState {
    id: string;
    originalName: string;
    draft: string;
}

type ElementType = CanvasElement["type"];

function elementIcon(type: ElementType): LucideIconType {
    switch (type) {
        case "image":
            return ImageIcon;
        case "video":
            return Video;
        case "carousel":
            return Images;
        case "rtmp":
            return Radio;
        case "text":
            return Type;
        case "web-browser":
            return Globe;
        default:
            return Box;
    }
}

export default function Elements() {
    const {
        status,
        canvas,
        canvasLoading,
        canvasError,
        moveCanvasElementZ,
        renameCanvasElement,
    } = useVideoWall();
    const [busy, setBusy] = useState(false);
    const [rename, setRename] = useState<RenameState | null>(null);
    const [localError, setLocalError] = useState<string | null>(null);

    const layers = [...canvas];//.reverse();

    async function moveElement(element: CanvasElement, direction: "up" | "down") {
        setBusy(true);
        setLocalError(null);
        try {
            await moveCanvasElementZ(element.id, direction);
        } catch {
            // The context owns and exposes the operation error for every consumer.
        } finally {
            setBusy(false);
        }
    }

    async function saveRename() {
        if (!rename) return;
        const newName = rename.draft.trim();
        if (!newName) {
            setLocalError("An element name cannot be empty.");
            return;
        }
        if (newName === rename.originalName) {
            setRename(null);
            setLocalError(null);
            return;
        }

        setBusy(true);
        setLocalError(null);
        try {
            await renameCanvasElement(rename.id, newName);
            setRename(null);
        } catch {
            // The context owns and exposes the operation error for every consumer.
        } finally {
            setBusy(false);
        }
    }

    function handleRenameKeyDown(event: KeyboardEvent<HTMLInputElement>) {
        if (event.key === "Enter") {
            event.preventDefault();
            void saveRename();
        } else if (event.key === "Escape") {
            setRename(null);
            setLocalError(null);
        }
    }

    const error = localError ?? canvasError;

    return (
        <>
            <div className="panel-header">
                <span>Layers</span>
                <div className="panel-header-actions">
                    <button
                        className="panel-header-button"
                        type="button"
                        disabled
                        title="Add an element (coming soon)"
                        aria-label="Add an element"
                    >
                        <Plus size={18}/>
                    </button>
                    <button
                        className="panel-header-button"
                        type="button"
                        disabled
                        title="Duplicate selected layer (coming soon)"
                        aria-label="Duplicate selected layer"
                    >
                        <Copy size={17}/>
                    </button>
                    <button
                        className="panel-header-button"
                        type="button"
                        disabled
                        title="Delete selected layer (coming soon)"
                        aria-label="Delete selected layer"
                    >
                        <Trash2 size={17}/>
                    </button>
                </div>
            </div>

            <div className="panel-content panel-scroll-content" aria-busy={canvasLoading || busy}>
                {status !== "connected" ? (
                    <p className="empty-state">Connect to a video wall to view its layers.</p>
                ) : canvasLoading ? (
                    <p className="empty-state">Loading layers…</p>
                ) : (
                    <>
                        {error && <p className="panel-error" role="alert">{error}</p>}
                        {layers.length === 0 && !error ? (
                            <p className="empty-state">No elements on this canvas.</p>
                        ) : (
                            <ul className="item-list">
                                {layers.map((element, index) => {
                                    const isEditing = rename !== null && rename.id === element.id;
                                    const ElementIcon = elementIcon(element.type);
                                    return (
                                        <li className="item-row" key={element.id}>
                                            <span className="item-type-icon" title={element.type}>
                                                <ElementIcon size={20}/>
                                            </span>

                                            {isEditing ? (
                                                <form
                                                    className="inline-edit-form"
                                                    onSubmit={(event) => {
                                                        event.preventDefault();
                                                        void saveRename();
                                                    }}
                                                >
                                                    <input
                                                        aria-label={`Rename ${element.name}`}
                                                        autoFocus
                                                        value={rename.draft}
                                                        disabled={busy}
                                                        onChange={(event) => setRename((current) => current ? {
                                                            ...current,
                                                            draft: event.target.value
                                                        } : current)}
                                                        onKeyDown={handleRenameKeyDown}
                                                    />
                                                    <button
                                                        className="icon-button"
                                                        type="submit"
                                                        disabled={busy || !rename.draft.trim()}
                                                        title="Save name"
                                                        aria-label="Save name"
                                                    >
                                                        <Check/>
                                                    </button>
                                                    <button
                                                        className="icon-button"
                                                        type="button"
                                                        disabled={busy}
                                                        onClick={() => setRename(null)}
                                                        title="Cancel rename"
                                                        aria-label="Cancel rename"
                                                    >
                                                        <X/>
                                                    </button>
                                                </form>
                                            ) : (
                                                <>
                                                    <span className="item-name" title={element.name}>
                                                        {element.name}
                                                    </span>
                                                    <button
                                                        className="icon-button item-edit-button"
                                                        type="button"
                                                        disabled={busy}
                                                        onClick={() => {
                                                            setLocalError(null);
                                                            setRename({
                                                                id: element.id,
                                                                originalName: element.name,
                                                                draft: element.name
                                                            });
                                                        }}
                                                        title={`Rename ${element.name}`}
                                                        aria-label={`Rename ${element.name}`}
                                                    >
                                                        <Pencil size={15}/>
                                                    </button>
                                                </>
                                            )}

                                            {!isEditing && (
                                                <div className="item-actions">
                                                    <button
                                                        className="icon-button"
                                                        type="button"
                                                        disabled={busy || index === 0}
                                                        onClick={() => void moveElement(element, "up")}
                                                        title="Move layer up"
                                                        aria-label={`Move ${element.name} up`}
                                                    >
                                                        <ChevronUp size={17}/>
                                                    </button>
                                                    <button
                                                        className="icon-button"
                                                        type="button"
                                                        disabled={busy || index === layers.length - 1}
                                                        onClick={() => void moveElement(element, "down")}
                                                        title="Move layer down"
                                                        aria-label={`Move ${element.name} down`}
                                                    >
                                                        <ChevronDown size={17}/>
                                                    </button>
                                                </div>
                                            )}
                                        </li>
                                    );
                                })}
                            </ul>
                        )}
                    </>
                )}
            </div>
        </>
    );
}
