import {useEffect, useRef, useState} from "react";
import {Circle, Layer, Stage} from "react-konva";
import {Stage as KonvaStage} from "konva/lib/Stage";
import type {KonvaEventObject} from "konva/lib/Node";

type PanState = {
    startX: number;
    startY: number;
    stageX: number;
    stageY: number;
};

export default function Canvas() {
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

    return (
        <div ref={containerRef} className="canvas-content">
            <Stage
                width={width}
                height={height}
                ref={stageRef}
                onWheel={handleWheel}
                onMouseDown={handleMouseDown}
            >
                <Layer>
                    <Circle
                        x={width / 2}
                        y={height / 2}
                        radius={50}
                        fill="green"
                    />
                </Layer>
            </Stage>
        </div>
    );
}
