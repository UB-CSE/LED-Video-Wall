import React, { useState, useEffect } from "react";
import { useDispatch } from "react-redux";
import {
  setSelectedElement,
  updateElement,
} from "../state/config/configSlice.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";

type ImageProps = {
  name: string;
  id: number;
  type: "image";
  path: string;
  location: [number, number];
  sizeMultiplier: number;
  zoomScale: number;
  scale: number;
  panOffset: { x: number; y: number };
};
type TextProps = {
  name: string;
  id: number;
  type: "text";
  content: string;
  size: number;
  color: string;
  font_path: string;
  location: [number, number];
  sizeMultiplier: number;
  zoomScale: number;
  panOffset: { x: number; y: number };
};
type PlaceholderProps = {
  name: string;
  id: number;
  type: "carousel" | "video" | "webcam" | "rtmp";
  location: [number, number];
  sizeMultiplier: number;
  zoomScale: number;
  panOffset: { x: number; y: number };
  size?: number[];
};
type ElementProps = ImageProps | TextProps | PlaceholderProps;

function Element(props: ElementProps) {
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  const [x, setX] = useState(0);
  const [y, setY] = useState(0);
  const [startX, setStartX] = useState(0);
  const [startY, setStartY] = useState(0);
  const [isDragging, setIsDragging] = useState(false);
  const [dimensions, setDimensions] = useState([0, 0]);
  const [fontLoaded, setFontLoaded] = useState(false);

  function updateState() {
    const canvasX = props.location[0] + x / props.zoomScale;
    const canvasY = props.location[1] + y / props.zoomScale;
    const current = configState.elements.find((el) => el.id === props.id);
    if (!current) return;
    dispatch(updateElement({ ...current, location: [canvasX, canvasY] }));
  }

  function startDragging(e: React.MouseEvent) {
    dispatch(setSelectedElement(props.id));
    setIsDragging(true);
    setStartX(e.clientX - x);
    setStartY(e.clientY - y);
  }

  function sendPosition() {
    fetch("/api/send-location", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        id: String(props.id),
        x: Math.trunc((props.location[0] + x / props.zoomScale) / props.sizeMultiplier),
        y: Math.trunc((props.location[1] + y / props.zoomScale) / props.sizeMultiplier),
      }),
    });
  }

  function handleLoad(e: React.SyntheticEvent<HTMLImageElement, Event>) {
    const { naturalHeight, naturalWidth } = e.currentTarget;
    setDimensions([
      naturalWidth * props.sizeMultiplier,
      naturalHeight * props.sizeMultiplier,
    ]);
  }

  useEffect(() => {
    if (isDragging) {
      function handleDrag(e: MouseEvent) {
        setX(e.clientX - startX);
        setY(e.clientY - startY);
      }
      function stopDragging() {
        setIsDragging(false);
      }
      document.addEventListener("mousemove", handleDrag);
      document.addEventListener("mouseup", stopDragging);
      return () => {
        document.removeEventListener("mousemove", handleDrag);
        document.removeEventListener("mouseup", stopDragging);
      };
    } else {
      updateState();
      sendPosition();
    }
  }, [isDragging]);

  useEffect(() => {
    setX(0);
    setY(0);
  }, [props.location[0], props.location[1]]);

  useEffect(() => {
    if (props.type === "text" && props.font_path) {
      setFontLoaded(false);
      const fontFileName = props.font_path.split("/").pop() || "";
      const fontUrl = `/api/fonts/${fontFileName}`;
      const fontFamilyName = `customFont${props.id}`;
      const font = new FontFace(fontFamilyName, `url(${fontUrl})`);
      font
        .load()
        .then((loadedFont) => {
          document.fonts.add(loadedFont);
          setFontLoaded(true);
        })
        .catch((error) => {
          console.error(`Failed to load font: ${fontFileName}`, error);
        });
    }
  }, [props.type === "text" ? props.font_path : null, props.id]);

  function createJSXElement() {
    const left = props.location[0] * props.zoomScale + x + props.panOffset.x;
    const top = props.location[1] * props.zoomScale + y + props.panOffset.y;

    if (props.type === "image") {
      return (
        <img
          src={"/api/" + props.path}
          draggable={false}
          onMouseDown={(e) => startDragging(e)}
          onLoad={handleLoad}
          style={{
            position: "fixed",
            left,
            top,
            cursor: isDragging ? "grabbing" : "grab",
            width: dimensions[0] * props.scale * props.zoomScale,
            height: dimensions[1] * props.scale * props.zoomScale,
            margin: "0px",
            zIndex: 100,
            border: configState.selectedElement === props.id ? "3px solid cornflowerblue" : "none",
          }}
        />
      );
    } else if (props.type === "text") {
      return (
        <div
          draggable={false}
          onMouseDown={(e) => startDragging(e)}
          style={{
            position: "fixed",
            left,
            top,
            cursor: isDragging ? "grabbing" : "grab",
            margin: "0px",
            zIndex: 100,
            border: configState.selectedElement === props.id ? "3px solid cornflowerblue" : "none",
          }}
        >
          <p
            style={{
              color: props.color,
              fontSize: props.size * props.sizeMultiplier * props.zoomScale,
              userSelect: "none",
              fontFamily: `customFont${props.id}, sans-serif`,
              visibility: fontLoaded ? "visible" : "hidden",
              margin: 0,
            }}
          >
            {props.content}
          </p>
        </div>
      );
    } else {
      const placeholderColors: Record<string, string> = {
        carousel: "#4a90d9",
        video:    "#7b5ea7",
        webcam:   "#2e8b57",
        rtmp:     "#c0392b",
      };
      const placeholderW = props.type === "rtmp" && props.size && props.size[0] > 0
        ? props.size[0] * props.sizeMultiplier * props.zoomScale
        : 64 * props.zoomScale;
      const placeholderH = props.type === "rtmp" && props.size && props.size[1] > 0
        ? props.size[1] * props.sizeMultiplier * props.zoomScale
        : 64 * props.zoomScale;
      const color = placeholderColors[props.type] ?? "#888";
      return (
        <div
          draggable={false}
          onMouseDown={(e) => startDragging(e)}
          style={{
            position: "fixed",
            left,
            top,
            width: placeholderW,
            height: placeholderH,
            cursor: isDragging ? "grabbing" : "grab",
            backgroundColor: color,
            opacity: 0.75,
            zIndex: 100,
            border: configState.selectedElement === props.id
              ? "3px solid cornflowerblue"
              : "2px dashed rgba(255,255,255,0.6)",
            boxSizing: "border-box",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            flexDirection: "column",
            gap: "4px",
          }}
        >
          <span style={{ color: "white", fontSize: 10 * props.zoomScale, fontWeight: "bold", userSelect: "none" }}>
            {props.type.toUpperCase()}
          </span>
          <span style={{ color: "rgba(255,255,255,0.8)", fontSize: 9 * props.zoomScale, userSelect: "none" }}>
            {props.name}
          </span>
        </div>
      );
    }
  }

  return <>{createJSXElement()}</>;
}

export default Element;