import {useCallback, useEffect, useRef, useState} from "react";
import {Check, ImagePlus, RefreshCw, Upload, X} from "lucide-react";

export interface ImageGalleryModalProps {
    open: boolean;
    onClose: () => void;
    onSelect?: (filepath: string) => void;
}

function normalizeImagePath(filepath: string): string {
    return filepath.trim().replace(/^\/?images\//, "").replace(/^\//, "");
}

function imageUrl(filepath: string): string {
    const encoded = normalizeImagePath(filepath).split("/").map((part) => encodeURIComponent(part)).join("/");
    return `/api/resources/${encoded}`;
}

function extractImageNames(payload: unknown): string[] {
    const values = Array.isArray(payload)
        ? payload
        : typeof payload === "object" && payload !== null && Array.isArray((payload as {images?: unknown}).images)
            ? (payload as {images: unknown[]}).images
            : null;

    if (!values) throw new Error("The image list response did not contain an images array.");

    const names = values.flatMap((entry): string[] => {
        if (typeof entry === "string") return [entry];
        if (typeof entry !== "object" || entry === null) return [];
        const record = entry as Record<string, unknown>;
        const filepath = record.filepath ?? record.filename ?? record.name;
        return typeof filepath === "string" ? [filepath] : [];
    });

    return [...new Set(names.map(normalizeImagePath).filter(Boolean))];
}

function getRandomImageName(file: File): string {
    const extension = file.name.match(/\.[a-zA-Z0-9]{1,12}$/)?.[0].toLowerCase() ?? "";
    const randomId = typeof crypto.randomUUID === "function"
        ? crypto.randomUUID()
        : Array.from(crypto.getRandomValues(new Uint8Array(16)), (value) => value.toString(16).padStart(2, "0")).join("");
    return `${randomId}${extension}`;
}

export default function ImageGalleryModal({open, onClose, onSelect}: ImageGalleryModalProps) {
    const [images, setImages] = useState<string[]>([]);
    const [selectedImage, setSelectedImage] = useState<string | null>(null);
    const [loading, setLoading] = useState(false);
    const [uploading, setUploading] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);

    const loadImages = useCallback(async () => {
        setLoading(true);
        setError(null);
        try {
            const response = await fetch("/api/list-images");
            if (!response.ok) throw new Error(`Could not load images (${response.status}).`);
            const payload: unknown = await response.json();
            if (typeof payload === "object" && payload !== null && (payload as {success?: unknown}).success === false) {
                const message = (payload as {message?: unknown; error?: unknown}).message ?? (payload as {error?: unknown}).error;
                throw new Error(typeof message === "string" ? message : "The server could not list images.");
            }
            const nextImages = extractImageNames(payload);
            setImages(nextImages);
            setSelectedImage((current) => current && nextImages.includes(current) ? current : null);
        } catch (loadError) {
            setError(loadError instanceof Error ? loadError.message : "Could not load images.");
        } finally {
            setLoading(false);
        }
    }, []);

    useEffect(() => {
        if (open) void loadImages();
    }, [open, loadImages]);

    useEffect(() => {
        if (!open) return;
        function handleKeyDown(event: KeyboardEvent) {
            if (event.key === "Escape") onClose();
        }
        window.addEventListener("keydown", handleKeyDown);
        return () => window.removeEventListener("keydown", handleKeyDown);
    }, [open, onClose]);

    async function uploadImage(file: File) {
        setUploading(true);
        setError(null);
        const randomFilename = getRandomImageName(file);
        try {
            const formData = new FormData();
            // The server receives a random filename; the user's original basename is not sent.
            formData.append("file", file, randomFilename);
            const response = await fetch("/api/upload-image", {method: "POST", body: formData});
            let payload: unknown = null;
            try {
                payload = await response.json();
            } catch {
                // A successful 204/empty response is also accepted; the generated filename is known.
            }
            if (!response.ok) {
                const message = typeof payload === "object" && payload !== null
                    ? (payload as {message?: unknown; error?: unknown}).message ?? (payload as {error?: unknown}).error
                    : null;
                throw new Error(typeof message === "string" ? message : `Image upload failed (${response.status}).`);
            }
            if (typeof payload === "object" && payload !== null && (payload as {success?: unknown}).success === false) {
                const message = (payload as {message?: unknown; error?: unknown}).message ?? (payload as {error?: unknown}).error;
                throw new Error(typeof message === "string" ? message : "The server could not upload the image.");
            }
            const record = typeof payload === "object" && payload !== null ? payload as Record<string, unknown> : {};
            const uploadedFilename = normalizeImagePath([record.filename, record.name, record.filepath]
                .find((value): value is string => typeof value === "string" && value.length > 0) ?? randomFilename);
            const nextImages = await fetch("/api/list-images").then(async (listResponse) => {
                if (!listResponse.ok) throw new Error("The image uploaded, but the gallery could not be refreshed.");
                return extractImageNames(await listResponse.json());
            });
            setImages(nextImages.includes(uploadedFilename) ? nextImages : [...nextImages, uploadedFilename]);
            setSelectedImage(uploadedFilename);
        } catch (uploadError) {
            setError(uploadError instanceof Error ? uploadError.message : "Image upload failed.");
        } finally {
            setUploading(false);
            if (fileInputRef.current) fileInputRef.current.value = "";
        }
    }

    if (!open) return null;

    return (
        <div className="modal-backdrop gallery-modal-backdrop" onMouseDown={(event) => {
            if (event.target === event.currentTarget) onClose();
        }}>
            <section className="modal-dialog image-gallery-dialog" role="dialog" aria-modal="true" aria-labelledby="image-gallery-title">
                <header className="modal-header">
                    <div>
                        <h2 id="image-gallery-title">Image gallery</h2>
                        <p>Choose an image from the server or upload a new one.</p>
                    </div>
                    <button className="icon-button modal-close-button" type="button" onClick={onClose} aria-label="Close image gallery">
                        <X size={19}/>
                    </button>
                </header>

                <div className="gallery-toolbar">
                    <button className="secondary-button" type="button" onClick={() => fileInputRef.current?.click()} disabled={uploading}>
                        <Upload size={16}/>
                        {uploading ? "Uploading…" : "Upload image"}
                    </button>
                    <button className="secondary-button" type="button" onClick={() => void loadImages()} disabled={loading || uploading}>
                        <RefreshCw size={15}/>
                        Refresh
                    </button>
                    <input
                        ref={fileInputRef}
                        className="visually-hidden"
                        type="file"
                        accept="image/*"
                        onChange={(event) => {
                            const file = event.target.files?.[0];
                            if (file) void uploadImage(file);
                        }}
                    />
                </div>

                {error && <p className="modal-error" role="alert">{error}</p>}
                <div className="image-gallery-grid" aria-busy={loading || uploading}>
                    {loading ? (
                        <p className="empty-state gallery-empty">Loading images…</p>
                    ) : images.length === 0 ? (
                        <p className="empty-state gallery-empty">No images found. Upload an image to get started.</p>
                    ) : images.map((filepath) => (
                        <button
                            className={`image-gallery-item${selectedImage === filepath ? " image-gallery-item-selected" : ""}`}
                            type="button"
                            key={filepath}
                            onClick={() => setSelectedImage(filepath)}
                            aria-pressed={selectedImage === filepath}
                            title={filepath}
                        >
                            <span className="image-gallery-thumbnail">
                                <img src={imageUrl(filepath)} alt="" loading="lazy"/>
                                {selectedImage === filepath && <span className="image-gallery-check"><Check size={15}/></span>}
                            </span>
                            <span className="image-gallery-filename">{filepath.split("/").pop()}</span>
                        </button>
                    ))}
                </div>

                <footer className="modal-footer">
                    <span className="modal-footer-note">{images.length} image{images.length === 1 ? "" : "s"}</span>
                    <button className="secondary-button" type="button" onClick={onClose}>Cancel</button>
                    {onSelect && (
                        <button className="primary-button" type="button" disabled={!selectedImage || loading || uploading} onClick={() => {
                            if (selectedImage) onSelect(selectedImage);
                        }}>
                            <ImagePlus size={16}/>
                            Use selected image
                        </button>
                    )}
                </footer>
            </section>
        </div>
    );
}
