import {useEffect, useRef, useState} from "react";
import {Layer, Rect, Stage} from "react-konva";
import {Stage as KonvaStage} from "konva/lib/Stage";
import type {KonvaEventObject} from "konva/lib/Node";
import {type Location, useVideoWall} from "../context/VideoWallContext.tsx";

type PanState = {
    startX: number;
    startY: number;
    stageX: number;
    stageY: number;
};

export default function Canvas() {
    const {
        status,
        matrices,
        canvasSize,
        canvas,
        canvasLoading,
        canvasError,
        moveCanvasElement,
    } = useVideoWall();

    const layers = [...canvas];//.reverse();

    const containerRef = useRef<HTMLDivElement>(null);
    const [width, setWidth] = useState(0);
    const [height, setHeight] = useState(0);
    const stageRef = useRef<KonvaStage>(null);
    const panRef = useRef<PanState | null>(null);

    useEffect(() => {
        const element = containerRef.current;
        if (!element) return;

        const observer = new ResizeObserver(([entry]) => {
            setWidth(entry.contentRect.width);
            setHeight(entry.contentRect.height);
        });

        observer.observe(element);
        return () => observer.disconnect();
    }, []);

    useEffect(() => {
        function handleMouseMove(e: MouseEvent) {
            const pan = panRef.current;
            const stage = stageRef.current;
            if (!pan || !stage) return;

            stage.position({
                x: pan.stageX + e.clientX - pan.startX,
                y: pan.stageY + e.clientY - pan.startY,
            });
        }

        function handleMouseUp() {
            panRef.current = null;
        }

        window.addEventListener("mousemove", handleMouseMove);
        window.addEventListener("mouseup", handleMouseUp);

        return () => {
            window.removeEventListener("mousemove", handleMouseMove);
            window.removeEventListener("mouseup", handleMouseUp);
        };
    }, []);

    function handleMouseDown(e: KonvaEventObject<MouseEvent>) {
        const evt = e.evt;
        const isMiddleButton = evt.button === 1;
        const isShiftLeftButton = evt.button === 0 && evt.shiftKey;

        if (!isMiddleButton && !isShiftLeftButton) return;

        evt.preventDefault();

        const stage = stageRef.current;
        if (!stage) return;

        panRef.current = {
            startX: evt.clientX,
            startY: evt.clientY,
            stageX: stage.x(),
            stageY: stage.y(),
        };
    }

    function handleWheel(e: KonvaEventObject<WheelEvent>) {
        e.evt.preventDefault();

        const stage = stageRef.current;
        if (!stage) return;

        const oldScale = stage.scaleX();
        const pointer = stage.getPointerPosition();
        if (!pointer) return;

        const mousePointTo = {
            x: (pointer.x - stage.x()) / oldScale,
            y: (pointer.y - stage.y()) / oldScale,
        };

        let direction = e.evt.deltaY > 0 ? 1 : -1;
        if (e.evt.ctrlKey) {
            direction = -direction;
        }

        const scaleBy = 1.02;
        let newScale = direction > 0 ? oldScale * scaleBy : oldScale / scaleBy;

        let newPos = {
            x: pointer.x - mousePointTo.x * newScale,
            y: pointer.y - mousePointTo.y * newScale,
        };

        if (Math.abs(e.evt.deltaX) > 0) {
            newPos = {
                x: stage.position().x - e.evt.deltaX / 2,
                y: stage.position().y - e.evt.deltaY / 2,
            };
            newScale = oldScale;
        }

        stage.position(newPos);
        stage.scale({x: newScale, y: newScale});
    }

    function getCanvasPos(location: Location): Location {
        return {
            x: width / 2 + location.x - canvasSize.width / 2,
            y: height / 2 + location.y - canvasSize.height / 2
        };
    }

    function fromCanvasPos(x: number, y: number): Location {
        return {
            x: x - width / 2 + canvasSize.width / 2,
            y: y - height / 2 + canvasSize.height / 2
        };
    }

    return (
        <>
            <div ref={containerRef} className="panel-content canvas-content" aria-busy={canvasLoading}>
                {status !== "connected" ? (
                    <p className="empty-state">No video wall connected.</p>
                ) : canvasLoading ? (
                    <p className="empty-state">Loading canvas…</p>
                ) : (
                    <>
                    {canvasError && <p className="panel-error" role="alert">{canvasError}</p>}
                        <Stage
                            width={width}
                            height={height}
                            ref={stageRef}
                            onWheel={handleWheel}
                            onMouseDown={handleMouseDown}
                        >
                            <Layer>
                                {matrices.map((mat, index) => {
                                    const pos = getCanvasPos(mat.location);
                                    return (
                                        <Rect
                                            x={pos.x}
                                            y={pos.y}
                                            width={mat.size.width}
                                            height={mat.size.height}
                                            fill="#2C2C2C"
                                            key={index}
                                        />
                                    );
                                })}
                            </Layer>

                            {layers.map((element,) => {
                                const pos = getCanvasPos(element.location);
                                return (
                                    <Layer>
                                        <Rect
                                            x={pos.x}
                                            y={pos.y}
                                            width={element.size.width}
                                            height={element.size.height}
                                            fill="#FF0000"
                                            key={element.id}
                                            draggable
                                            onMouseEnter={() => {
                                                document.body.style.cursor = 'pointer';
                                            }}
                                            onMouseLeave={() => {
                                                document.body.style.cursor = 'default';
                                            }}
                                            onDragEnd={(e) => {
                                                void moveCanvasElement(element.id, fromCanvasPos(e.target.x(), e.target.y()));
                                            }}
                                        />

                                    </Layer>
                                );
                            })}

                        </Stage>
                    </>
                )}
            </div>
        </>
    );
}
