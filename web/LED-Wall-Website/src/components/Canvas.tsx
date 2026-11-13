import {useCallback, useEffect, useRef, useState} from "react";
import {Group, Image as KonvaImage, Layer, Rect, Stage, Text, Transformer} from "react-konva";
import type {Stage as KonvaStage} from "konva/lib/Stage";
import type {Transformer as KonvaTransformer} from "konva/lib/shapes/Transformer";
import type {Node as KonvaNode} from "konva/lib/Node";
import type {Image as KonvaImageNode} from "konva/lib/shapes/Image";
import type {Rect as KonvaRectNode} from "konva/lib/shapes/Rect";
import type {KonvaEventObject} from "konva/lib/Node";
import {
    type CanvasElement,
    type ImageCanvasElement,
    type Location,
    type Size,
    useVideoWall
} from "../context/VideoWallContext.tsx";

type PanState = {
    startX: number;
    startY: number;
    stageX: number;
    stageY: number;
};

type ImageElementProps = {
    element: ImageCanvasElement;
    x: number;
    y: number;
    onSelect: () => void;
    onDragEnd: (node: KonvaNode) => void;
    onTransformEnd: (node: KonvaNode, afterReset?: (size: Size) => void) => void;
    registerNode: (element: CanvasElement, node: KonvaNode | null) => void;
};

function getImageURL(filepath: string): string {
    const normalized = filepath.replace(/^\/+/, "");
    const encoded = normalized.split("/").map((segment) => encodeURIComponent(segment)).join("/");
    return `/api/resources/${encoded}`;
}

function getContainedImageSize(imageWidth: number, imageHeight: number, box: Size): Size {
    if (imageWidth <= 0 || imageHeight <= 0 || box.width <= 0 || box.height <= 0) {
        return {width: 0, height: 0};
    }

    const scale = Math.min(box.width / imageWidth, box.height / imageHeight);
    return {
        width: imageWidth * scale,
        height: imageHeight * scale,
    };
}

function CanvasImageElement({element, x, y, onSelect, onDragEnd, onTransformEnd, registerNode}: ImageElementProps) {
    const [image, setImage] = useState<HTMLImageElement | null>(null);
    const boundsRef = useRef<KonvaRectNode>(null);
    const imageRef = useRef<KonvaImageNode>(null);
    const groupRef = useRef<KonvaNode>(null);

    useEffect(() => {
        if (!element.filepath) {
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
        loadedImage.src = getImageURL(element.filepath);

        return () => {
            active = false;
            loadedImage.onload = null;
            loadedImage.onerror = null;
        };
    }, [element.filepath]);

    const getRenderedImageSize = (box: Size): Size => {
        if (!image || !element.preserveAspectRatio) return box;
        return getContainedImageSize(image.naturalWidth || image.width, image.naturalHeight || image.height, box);
    };

    const syncImageToBounds = (box: Size) => {
        const imageNode = imageRef.current;
        if (!imageNode || !image) return;

        const renderedSize = getRenderedImageSize(box);
        imageNode.setAttrs({
            x: 0,
            y: 0,
            width: renderedSize.width,
            height: renderedSize.height,
            image: image,
        });
        imageNode.getLayer()?.batchDraw();
    };

    const syncImageDuringTransform = () => {
        if (!image || !element.preserveAspectRatio) return;
        const group = groupRef.current;
        const imageNode = imageRef.current;
        if (!group || !imageNode) return;

        const scaleX = Math.max(0.0001, Math.abs(group.scaleX()));
        const scaleY = Math.max(0.0001, Math.abs(group.scaleY()));
        const transformedBounds = {
            width: group.width() * scaleX,
            height: group.height() * scaleY,
        };
        const renderedSize = getContainedImageSize(
            image.naturalWidth || image.width,
            image.naturalHeight || image.height,
            transformedBounds,
        );
        // The group itself is scaled by Konva's Transformer. Counter-scale the
        // image's local geometry so the pixels remain uniformly scaled on screen.
        const localSize = {
            width: renderedSize.width / scaleX,
            height: renderedSize.height / scaleY,
        };

        imageNode.setAttrs({
            x: 0,
            y: 0,
            width: localSize.width,
            height: localSize.height,
            image: image,
        });
        imageNode.getLayer()?.batchDraw();
    };

    const initialImageSize = getRenderedImageSize(element.size);

    return (
        <Group
            ref={(node) => {
                groupRef.current = node;
                registerNode(element, node);
            }}
            x={x}
            y={y}
            width={element.size.width}
            height={element.size.height}
            offsetX={element.size.width / 2}
            offsetY={element.size.height / 2}
            rotation={-element.rotation}
            draggable
            onClick={onSelect}
            onTap={onSelect}
            onDragStart={onSelect}
            onDragEnd={(event) => onDragEnd(event.target)}
            onTransform={syncImageDuringTransform}
            onTransformEnd={(event) => onTransformEnd(event.target, (size) => {
                boundsRef.current?.size(size);
                syncImageToBounds(size);
            })}
        >
            {/* The transparent rectangle is the element's logical bounds. It remains
                the selection/resize target even when the contained image has margins. */}
            <Rect
                ref={boundsRef}
                x={0}
                y={0}
                width={element.size.width}
                height={element.size.height}
                fill={image ? "rgba(0, 0, 0, 0.001)" : "#ff0000"}
            />
            {image && (
                <KonvaImage
                    ref={imageRef}
                    image={image}
                    x={0}
                    y={0}
                    width={initialImageSize.width}
                    height={initialImageSize.height}
                    listening={false}
                />
            )}
        </Group>
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

    const layers = [...canvas].reverse();

    const registerNode = useCallback((element: CanvasElement, node: KonvaNode | null) => {
        if (node) elementNodesRef.current.set(element.id, node);
        else elementNodesRef.current.delete(element.id);

        if (selectedCanvasElementId === element.id && transformerRef.current) {
            transformerRef.current.nodes(node ? [node] : []);
            transformerRef.current.getLayer()?.batchDraw();
            transformerRef.current.resizeEnabled(element.type != "text");
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
    }, [selectedCanvasElementId, layers, canvasLoading, width, height]);

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

    function handleElementTransformEnd(
        element: CanvasElement,
        node: KonvaNode,
        afterReset?: (size: Size) => void,
    ) {
        const nextSize: Size = {
            width: Math.max(1, node.width() * Math.abs(node.scaleX())),
            height: Math.max(1, node.height() * Math.abs(node.scaleY())),
        };
        const nextRotationCCW = -node.rotation();

        node.width(nextSize.width);
        node.height(nextSize.height);
        node.scaleX(1);
        node.scaleY(1);
        node.offsetX(nextSize.width / 2);
        node.offsetY(nextSize.height / 2);
        afterReset?.(nextSize);
        node.getLayer()?.batchDraw();

        void transformCanvasElement(element.id, nextSize, nextRotationCCW);
    }

    const selectedElement = layers.find((element) => element.id === selectedCanvasElementId);

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
                            {layers.map((element) => {
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
                                            onTransformEnd={(node, afterReset) => handleElementTransformEnd(element, node, afterReset)}
                                            registerNode={registerNode}
                                        />
                                    );
                                }

                                if (element.type === "text") {
                                    const hexColor =
                                        "#" + [element.color.r, element.color.g, element.color.b]
                                            .map((value: number) => value.toString(16).padStart(2, "0"))
                                            .join("");

                                    return (
                                        <Text
                                            key={element.id}
                                            ref={(node) => registerNode(element, node)}
                                            x={centerX}
                                            y={centerY}
                                            //width={element.size.width}
                                            //height={element.size.height}
                                            offsetX={element.size.width / 2}
                                            offsetY={element.size.height / 2}
                                            rotation={-element.rotation}
                                            text={element.text}
                                            fontSize={element.fontSize}
                                            fill={hexColor}
                                            align="left"
                                            verticalAlign="top"
                                            fontFamily={`"${element.fontPath}"`}
                                            wrap="none"
                                            padding={0}
                                            draggable
                                            onClick={onSelect}
                                            onTap={onSelect}
                                            onDragStart={onSelect}
                                            onDragEnd={(event) => onDragEnd(event.target)}
                                            //onTransformEnd={(event) => handleElementTransformEnd(element, event.target)}
                                        />
                                    );
                                }

                                return (
                                    <Rect
                                        key={element.id}
                                        ref={(node) => registerNode(element, node)}
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
