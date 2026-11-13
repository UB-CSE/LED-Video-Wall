import {useEffect, useState, type FormEvent} from "react";
import {Image as ImageIcon, X} from "lucide-react";
import {type RGB, useVideoWall} from "../context/VideoWallContext.tsx";
import ImageGalleryModal from "./ImageGalleryModal.tsx";

type ElementType = "image" | "text";

interface CreateElementModalProps {
    open: boolean;
    onClose: () => void;
}

const DEFAULT_COLOR: RGB = {r: 255, g: 255, b: 255};

function imageURL(filepath: string): string {
    const normalized = filepath.trim().replace(/^\/?images\//, "").replace(/^\//, "");
    return `/api/resources/${normalized.split("/").map((part) => encodeURIComponent(part)).join("/")}`;
}

export default function CreateElementModal({open, onClose}: CreateElementModalProps) {
    const {status, fontNames, createImageElement, createTextElement} = useVideoWall();
    const [elementType, setElementType] = useState<ElementType>("image");
    const [text, setText] = useState("");
    const [fontPath, setFontPath] = useState("");
    const [fontSize, setFontSize] = useState("24");
    const [color, setColor] = useState<RGB>(DEFAULT_COLOR);
    const [selectedImage, setSelectedImage] = useState<string | null>(null);
    const [galleryOpen, setGalleryOpen] = useState(false);
    const [busy, setBusy] = useState(false);
    const [error, setError] = useState<string | null>(null);

    useEffect(() => {
        if (fontNames.length > 0 && !fontNames.includes(fontPath)) setFontPath(fontNames[0]);
    }, [fontNames, fontPath]);

    useEffect(() => {
        if (!open) return;
        function handleKeyDown(event: KeyboardEvent) {
            if (event.key === "Escape" && !galleryOpen) onClose();
        }
        window.addEventListener("keydown", handleKeyDown);
        return () => window.removeEventListener("keydown", handleKeyDown);
    }, [open, galleryOpen, onClose]);

    async function submit(event: FormEvent<HTMLFormElement>) {
        event.preventDefault();
        setError(null);
        if (status !== "connected") {
            setError("Connect to a video wall before creating an element.");
            return;
        }

        setBusy(true);
        try {
            if (elementType === "image") {
                if (!selectedImage) throw new Error("Choose an image from the gallery first.");
                await createImageElement(selectedImage);
            } else {
                if (!text.trim()) throw new Error("Enter some text for the new element.");
                if (!fontPath) throw new Error("Choose a font.");
                const parsedSize = Number(fontSize);
                if (!Number.isFinite(parsedSize) || parsedSize <= 0) throw new Error("Font size must be greater than zero.");
                await createTextElement(text, fontPath, parsedSize, color);
            }
            onClose();
        } catch (submitError) {
            setError(submitError instanceof Error ? submitError.message : "Could not create the element.");
        } finally {
            setBusy(false);
        }
    }

    if (!open) return null;

    return (
        <>
            <div className="modal-backdrop" onMouseDown={(event) => {
                if (event.target === event.currentTarget && !busy) onClose();
            }}>
                <section className="modal-dialog create-element-dialog" role="dialog" aria-modal="true" aria-labelledby="create-element-title">
                    <header className="modal-header">
                        <div>
                            <h2 id="create-element-title">Create element</h2>
                            <p>Add a new element to the current canvas.</p>
                        </div>
                        <button className="icon-button modal-close-button" type="button" onClick={onClose} disabled={busy} aria-label="Close create element dialog">
                            <X size={19}/>
                        </button>
                    </header>

                    <form onSubmit={(event) => void submit(event)}>
                        <div className="modal-body">
                            <label className="form-field">
                                <span>Element type</span>
                                <select value={elementType} onChange={(event) => {
                                    setElementType(event.target.value as ElementType);
                                    setError(null);
                                }} disabled={busy}>
                                    <option value="image">Image</option>
                                    <option value="text">Text</option>
                                </select>
                            </label>

                            {elementType === "image" ? (
                                <div className="form-field">
                                    <span>Image</span>
                                    {selectedImage ? (
                                        <div className="selected-image-preview">
                                            <img src={imageURL(selectedImage)} alt="Selected image preview"/>
                                            <div className="selected-image-info">
                                                <strong>{selectedImage.split("/").pop()}</strong>
                                                <span>{selectedImage}</span>
                                            </div>
                                        </div>
                                    ) : (
                                        <div className="image-picker-empty">No image selected.</div>
                                    )}
                                    <button className="secondary-button image-picker-button" type="button" onClick={() => setGalleryOpen(true)} disabled={busy}>
                                        <ImageIcon size={16}/>
                                        {selectedImage ? "Choose a different image" : "Choose from image gallery"}
                                    </button>
                                    <small className="form-help">Upload a new image or select one already stored on the server.</small>
                                </div>
                            ) : (
                                <>
                                    <label className="form-field">
                                        <span>Text content</span>
                                        <textarea value={text} onChange={(event) => setText(event.target.value)} rows={4} disabled={busy} placeholder="Enter the text to display" required/>
                                    </label>
                                    <div className="form-field-row">
                                        <label className="form-field form-field-grow">
                                            <span>Font name</span>
                                            <select value={fontPath} onChange={(event) => setFontPath(event.target.value)} disabled={busy || fontNames.length === 0} required>
                                                {fontNames.length === 0 ? <option value="">No fonts available</option> : fontNames.map((name) => <option key={name} value={name}>{name}</option>)}
                                            </select>
                                        </label>
                                        <label className="form-field font-size-field">
                                            <span>Font size (px)</span>
                                            <input type="number" min="0.1" step="any" value={fontSize} onChange={(event) => setFontSize(event.target.value)} disabled={busy} required/>
                                        </label>
                                    </div>
                                    <fieldset className="rgb-fieldset">
                                        <legend>Text color (RGB)</legend>
                                        <div className="rgb-inputs">
                                            {(["r", "g", "b"] as const).map((channel) => (
                                                <label className={`form-field rgb-field rgb-${channel}`} key={channel}>
                                                    <span>{channel.toUpperCase()}</span>
                                                    <input
                                                        type="number"
                                                        min="0"
                                                        max="255"
                                                        step="1"
                                                        value={color[channel]}
                                                        onChange={(event) => setColor((current) => ({...current, [channel]: Math.max(0, Math.min(255, Number(event.target.value))) }))}
                                                        disabled={busy}
                                                        required
                                                    />
                                                </label>
                                            ))}
                                            <span className="rgb-preview" style={{backgroundColor: `rgb(${color.r}, ${color.g}, ${color.b})`}} aria-label={`Color preview RGB ${color.r}, ${color.g}, ${color.b}`}/>
                                        </div>
                                    </fieldset>
                                </>
                            )}

                            {error && <p className="modal-error" role="alert">{error}</p>}
                        </div>

                        <footer className="modal-footer">
                            <span className="modal-footer-note">{status === "connected" ? "Connected to video wall" : "Not connected"}</span>
                            <button className="secondary-button" type="button" onClick={onClose} disabled={busy}>Cancel</button>
                            <button className="primary-button" type="submit" disabled={busy || status !== "connected" || (elementType === "image" && !selectedImage) || (elementType === "text" && (!text.trim() || !fontPath))}>
                                {busy ? "Creating…" : "Create element"}
                            </button>
                        </footer>
                    </form>
                </section>
            </div>

            <ImageGalleryModal
                open={galleryOpen}
                onClose={() => setGalleryOpen(false)}
                onSelect={(filepath) => {
                    setSelectedImage(filepath);
                    setGalleryOpen(false);
                    setError(null);
                }}
            />
        </>
    );
}
