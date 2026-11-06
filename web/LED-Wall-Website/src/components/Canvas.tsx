import {useCallback, useEffect, useRef, useState} from "react";
import {Image as KonvaImage, Layer, Rect, Stage, Text, Transformer} from "react-konva";
import type {Stage as KonvaStage} from "konva/lib/Stage";
import type {Transformer as KonvaTransformer} from "konva/lib/shapes/Transformer";
import type {Node as KonvaNode} from "konva/lib/Node";
import type {KonvaEventObject} from "konva/lib/Node";
import {type CanvasElement, type Location, type Size, useVideoWall} from "../context/VideoWallContext.tsx";

type PanState = {
    startX: number;
    startY: number;
    stageX: number;
    stageY: number;
};

type ImageElementProps = {
    element: CanvasElement;
    x: number;
    y: number;
    onSelect: () => void;
    onDragEnd: (node: KonvaNode) => void;
    onTransformEnd: (node: KonvaNode) => void;
    registerNode: (id: string, node: KonvaNode | null) => void;
};

function getImageUrl(filepath: string): string {
    const normalized = filepath.replace(/^\/+/, "");
    const encoded = normalized.split("/").map((segment) => encodeURIComponent(segment)).join("/");
    return `/api/resources/${encoded}`;
}

function CanvasImageElement({element, x, y, onSelect, onDragEnd, onTransformEnd, registerNode}: ImageElementProps) {
    const [image, setImage] = useState<HTMLImageElement | null>(null);
    const filepath = typeof element.filepath === "string" ? element.filepath : "";

    useEffect(() => {
        if (!filepath) {
            setImage(null);
            return;
        }

        let active = true;
        const loadedImage = new window.Image();
        loadedImage.onload = () => {
            if (active) setImage(loadedImage);
        };
        loadedImage.onerror = () => {
            if (active) setImage(null);
        };
        loadedImage.src = getImageUrl(filepath);

        return () => {
            active = false;
            loadedImage.onload = null;
            loadedImage.onerror = null;
        };
    }, [filepath]);

    const sharedProps = {
        x,
        y,
        width: element.size.width,
        height: element.size.height,
        offsetX: element.size.width / 2,
        offsetY: element.size.height / 2,
        rotation: -element.rotation,
        draggable: true,
        onClick: onSelect,
        onTap: onSelect,
        onDragStart: onSelect,
        onDragEnd: (event: KonvaEventObject<DragEvent>) => onDragEnd(event.target),
        onTransformEnd: (event: KonvaEventObject<Event>) => onTransformEnd(event.target),
    };

    if (!image) {
        return (
            <Rect
                ref={(node) => registerNode(element.id, node)}
                {...sharedProps}
                fill="#ff0000"
            />
        );
    }

    return (
        <KonvaImage
            ref={(node) => registerNode(element.id, node)}
            {...sharedProps}
            image={image}
        />
    );
}

export default function Canvas() {
    const {
        status,
        matrices,
        canvasSize,
        canvas,
        canvasLoading,
        canvasError,
        selectedCanvasElementId,
        selectCanvasElement,
        moveCanvasElement,
        transformCanvasElement,
    } = useVideoWall();

    const containerRef = useRef<HTMLDivElement>(null);
    const [width, setWidth] = useState(0);
    const [height, setHeight] = useState(0);
    const stageRef = useRef<KonvaStage>(null);
    const transformerRef = useRef<KonvaTransformer>(null);
    const panRef = useRef<PanState | null>(null);
    const elementNodesRef = useRef<Map<string, KonvaNode>>(new Map());

    const registerNode = useCallback((id: string, node: KonvaNode | null) => {
        if (node) elementNodesRef.current.set(id, node);
        else elementNodesRef.current.delete(id);

        if (selectedCanvasElementId === id && transformerRef.current) {
            transformerRef.current.nodes(node ? [node] : []);
            transformerRef.current.getLayer()?.batchDraw();
        }
    }, [selectedCanvasElementId]);

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

    useEffect(() => {
        const transformer = transformerRef.current;
        if (!transformer) return;

        const selectedNode = selectedCanvasElementId
            ? elementNodesRef.current.get(selectedCanvasElementId)
            : undefined;
        transformer.nodes(selectedNode ? [selectedNode] : []);
        transformer.getLayer()?.batchDraw();
    }, [selectedCanvasElementId, canvas, canvasLoading, width, height]);

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
        if (e.evt.ctrlKey) direction = -direction;
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
            y: height / 2 + location.y - canvasSize.height / 2,
        };
    }

    function fromCanvasPos(x: number, y: number): Location {
        return {
            x: x - width / 2 + canvasSize.width / 2,
            y: y - height / 2 + canvasSize.height / 2,
        };
    }

    function handleElementDragEnd(element: CanvasElement, node: KonvaNode) {
        const location = fromCanvasPos(
            node.x() - element.size.width / 2,
            node.y() - element.size.height / 2,
        );
        void moveCanvasElement(element.id, location);
    }

    function handleElementTransformEnd(element: CanvasElement, node: KonvaNode) {
        const nextSize: Size = {
            width: Math.max(1, node.width() * node.scaleX()),
            height: Math.max(1, node.height() * node.scaleY()),
        };
        const nextRotationCCW = -node.rotation();
        const nextLocation = fromCanvasPos(
            node.x() - nextSize.width / 2,
            node.y() - nextSize.height / 2,
        );

        node.width(nextSize.width);
        node.height(nextSize.height);
        node.scaleX(1);
        node.scaleY(1);
        node.offsetX(nextSize.width / 2);
        node.offsetY(nextSize.height / 2);
        node.getLayer()?.batchDraw();

        void transformCanvasElement(element.id, nextSize, nextLocation, nextRotationCCW);
    }

    const selectedElement = canvas.find((element) => element.id === selectedCanvasElementId);

    return (
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
                        onClick={(event) => {
                            if (event.target === event.target.getStage()) selectCanvasElement(null);
                        }}
                        onTap={(event) => {
                            if (event.target === event.target.getStage()) selectCanvasElement(null);
                        }}
                    >
                        <Layer listening={false}>
                            {matrices.map((matrix, index) => {
                                const pos = getCanvasPos(matrix.location);
                                return (
                                    <Rect
                                        x={pos.x}
                                        y={pos.y}
                                        width={matrix.size.width}
                                        height={matrix.size.height}
                                        fill="#2C2C2C"
                                        key={index}
                                    />
                                );
                            })}
                        </Layer>

                        <Layer>
                            {canvas.map((element) => {
                                const topLeft = getCanvasPos(element.location);
                                const centerX = topLeft.x + element.size.width / 2;
                                const centerY = topLeft.y + element.size.height / 2;
                                const onSelect = () => selectCanvasElement(element.id);
                                const onDragEnd = (node: KonvaNode) => handleElementDragEnd(element, node);

                                if (element.type === "image") {
                                    return (
                                        <CanvasImageElement
                                            key={element.id}
                                            element={element}
                                            x={centerX}
                                            y={centerY}
                                            onSelect={onSelect}
                                            onDragEnd={onDragEnd}
                                            onTransformEnd={(node) => handleElementTransformEnd(element, node)}
                                            registerNode={registerNode}
                                        />
                                    );
                                }

                                if (element.type === "text") {
                                    const content = typeof element.text === "string" ? element.text : "";
                                    return (
                                        <Text
                                            key={element.id}
                                            ref={(node) => registerNode(element.id, node)}
                                            x={centerX}
                                            y={centerY}
                                            width={element.size.width}
                                            height={element.size.height}
                                            offsetX={element.size.width / 2}
                                            offsetY={element.size.height / 2}
                                            rotation={-element.rotation}
                                            text={content}
                                            fontSize={18}
                                            fill="#171717"
                                            align="center"
                                            verticalAlign="middle"
                                            wrap="word"
                                            draggable
                                            onClick={onSelect}
                                            onTap={onSelect}
                                            onDragStart={onSelect}
                                            onDragEnd={(event) => onDragEnd(event.target)}
                                            onTransformEnd={(event) => handleElementTransformEnd(element, event.target)}
                                        />
                                    );
                                }

                                return (
                                    <Rect
                                        key={element.id}
                                        ref={(node) => registerNode(element.id, node)}
                                        x={centerX}
                                        y={centerY}
                                        width={element.size.width}
                                        height={element.size.height}
                                        offsetX={element.size.width / 2}
                                        offsetY={element.size.height / 2}
                                        rotation={-element.rotation}
                                        fill="#ff0000"
                                        draggable
                                        onClick={onSelect}
                                        onTap={onSelect}
                                        onDragStart={onSelect}
                                        onDragEnd={(event) => onDragEnd(event.target)}
                                        onTransformEnd={(event) => handleElementTransformEnd(element, event.target)}
                                    />
                                );
                            })}
                        </Layer>

                        <Layer>
                            <Transformer
                                ref={transformerRef}
                                rotateEnabled
                                keepRatio={selectedElement?.preserveAspectRatio ?? false}
                                flipEnabled={false}
                                rotateAnchorOffset={28}
                                boundBoxFunc={(oldBox, newBox) => {
                                    if (newBox.width < 5 || newBox.height < 5) return oldBox;
                                    return newBox;
                                }}
                            />
                        </Layer>
                    </Stage>
                </>
            )}
        </div>
    );
}
