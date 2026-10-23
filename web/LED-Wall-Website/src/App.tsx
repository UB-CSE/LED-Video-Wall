import {useRef, useState} from "react";
import "./App.css";
import Header from "./Header.tsx"
import Properties from "./Properties.tsx";
import Elements from "./Elements.tsx";
import Canvas from "./Canvas.tsx";

type ResizeState =
    | {
    type: "side";
    startX: number;
    startWidth: number;
}
    | {
    type: "vertical";
    startY: number;
    startHeight: number;
};

const MIN_SIDE_WIDTH = 220;
const MIN_PANEL_HEIGHT = 100;

export default function App() {
    const workspaceRef = useRef<HTMLElement>(null);
    const sideRegionRef = useRef<HTMLElement>(null);

    const [sideWidth, setSideWidth] = useState(320);
    const [topHeight, setTopHeight] = useState(50);
    const [resize, setResize] = useState<ResizeState | null>(null);

    function startSideResize(event: React.PointerEvent<HTMLDivElement>) {
        const width = sideRegionRef.current?.getBoundingClientRect().width;
        if (width === undefined) return;

        event.currentTarget.setPointerCapture(event.pointerId);
        setResize({type: "side", startX: event.clientX, startWidth: width});
    }

    function startVerticalResize(event: React.PointerEvent<HTMLDivElement>) {
        const height = sideRegionRef.current
            ?.querySelector(".side-panel")
            ?.getBoundingClientRect().height;

        if (height === undefined) return;

        event.currentTarget.setPointerCapture(event.pointerId);
        setResize({type: "vertical", startY: event.clientY, startHeight: height});
    }

    function handlePointerMove(event: React.PointerEvent<HTMLDivElement>) {
        if (!resize) return;

        if (resize.type === "side") {
            const workspaceWidth =
                workspaceRef.current?.getBoundingClientRect().width ?? window.innerWidth;
            const maxWidth = Math.max(MIN_SIDE_WIDTH, workspaceWidth - 250);
            const width = resize.startWidth - (event.clientX - resize.startX);

            setSideWidth(Math.max(MIN_SIDE_WIDTH, Math.min(width, maxWidth)));
            return;
        }

        const sideHeight =
            sideRegionRef.current?.getBoundingClientRect().height ?? 0;
        if (!sideHeight) return;

        const maxHeight = Math.max(MIN_PANEL_HEIGHT, sideHeight - MIN_PANEL_HEIGHT);
        const height = resize.startHeight + (event.clientY - resize.startY);
        const clampedHeight = Math.max(
            MIN_PANEL_HEIGHT,
            Math.min(height, maxHeight),
        );

        setTopHeight((clampedHeight / sideHeight) * 100);
    }

    const resizing = resize?.type ?? "";

    return (
        <div
            className={`app ${resizing ? `resizing-${resizing}` : ""}`}
            onPointerMove={handlePointerMove}
            onPointerUp={() => setResize(null)}
            onPointerCancel={() => setResize(null)}
        >
            <header className="page-header">
                <Header/>
            </header>

            <main
                className="workspace"
                ref={workspaceRef}
                style={{"--side-width": `${sideWidth}px`} as React.CSSProperties}
            >
                <section className="main-region">
                    <div className="panel-title">Canvas</div>
                    <div className="panel-content">
                        <Canvas/>
                    </div>
                </section>

                <div
                    className="resize-handle resize-horizontal"
                    role="separator"
                    aria-label="Resize side region"
                    aria-orientation="vertical"
                    onPointerDown={startSideResize}
                />

                <aside
                    className="side-region"
                    ref={sideRegionRef}
                    style={{"--top-height": `${topHeight}%`} as React.CSSProperties}
                >
                    <section className="side-panel">
                        <div className="panel-title">Properties</div>
                        <div className="panel-content">
                            <Properties/>
                        </div>
                    </section>

                    <div
                        className="resize-handle resize-vertical"
                        role="separator"
                        aria-label="Resize top side region"
                        aria-orientation="horizontal"
                        onPointerDown={startVerticalResize}
                    />

                    <section className="side-panel">
                        <div className="panel-title">Elements</div>
                        <div className="panel-content">
                            <Elements/>
                        </div>
                    </section>
                </aside>
            </main>
        </div>
    );
}
