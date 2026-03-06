import React, { useState, useRef } from "react";
import styles from "../Styles.module.css";
import uploadFile from "./Upload.tsx";
import { useDispatch, useSelector } from "react-redux";
import type { RootState } from "../state/store";
import type { Elem } from "../state/config/configSlice.ts";
import Element from "./element.tsx";

type Props = {
  canvasDimensions: number[];
  sizeMultiplier: number;
};

const ZOOM_IN_SCALE = 2.5;

function Canvas(props: Props) {
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

  const [scale, setScale] = useState(1);
  const [pan, setPan] = useState({ x: 0, y: 0 });
  const isZoomedIn = scale > 1;

  const isPanning = useRef(false);
  const panStart = useRef({ x: 0, y: 0 });
  const panOrigin = useRef({ x: 0, y: 0 });

  function handleZoomIn() {
    setScale(ZOOM_IN_SCALE);
    setPan({ x: 0, y: 0 });
  }

  function handleZoomOut() {
    setScale(1);
    setPan({ x: 0, y: 0 });
  }

  async function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    if (!e.dataTransfer.files || e.dataTransfer.files.length === 0) return;
    const canvasRect = e.currentTarget.getBoundingClientRect();
    let relativeX = e.clientX - canvasRect.left;
    relativeX = Math.trunc(relativeX / props.sizeMultiplier);
    let relativeY = e.clientY - canvasRect.top;
    relativeY = Math.trunc(relativeY / props.sizeMultiplier);
    uploadFile([relativeX, relativeY], e.dataTransfer.files[0], dispatch, configState);
  }

  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
  }

  function createJSXElement(element: Elem) {
    if (element.visible === false) return null;
    if (element.type === "image") {
      return (
        <Element
          key={element.id}
          name={element.name}
          id={element.id}
          type={element.type}
          path={element.filepath}
          location={[element.location[0], element.location[1]]}
          sizeMultiplier={props.sizeMultiplier}
          zoomScale={scale}
          scale={element.scale}
          panOffset={pan}
        />
      );
    } else if (element.type === "text") {
      return (
        <Element
          key={element.id}
          name={element.name}
          id={element.id}
          type={element.type}
          content={element.content}
          size={element.size}
          color={element.color}
          font_path={element.font_path}
          location={[element.location[0], element.location[1]]}
          sizeMultiplier={props.sizeMultiplier}
          zoomScale={scale}
          panOffset={pan}
        />
      );
    }
  }

  return (
    <>
      {/* Zoom controls - fixed to bottom center, above everything */}
      {props.canvasDimensions[1] > 0 && (
        <div
          style={{
            position: "fixed",
            bottom: "16px",
            left: "50%",
            transform: "translateX(-50%)",
            display: "flex",
            alignItems: "center",
            gap: "6px",
            zIndex: 1000,
            backgroundColor: "rgba(29,41,58,0.85)",
            borderRadius: "6px",
            padding: "4px 10px",
          }}
          onMouseDown={(e) => e.stopPropagation()}
        >
          <button
            onClick={handleZoomOut}
            disabled={!isZoomedIn}
            style={{ left: "unset", transform: "none", margin: 0, padding: "2px 10px", opacity: isZoomedIn ? 1 : 0.45 }}
          >
            −
          </button>
          <span style={{ color: "whitesmoke", fontSize: "13px", minWidth: "72px", textAlign: "center", userSelect: "none" }}>
            {isZoomedIn ? `Zoom: ${ZOOM_IN_SCALE}×` : "Zoom: 1×"}
          </span>
          <button
            onClick={handleZoomIn}
            disabled={isZoomedIn}
            style={{ left: "unset", transform: "none", margin: 0, padding: "2px 10px", opacity: isZoomedIn ? 0.45 : 1 }}
          >
            +
          </button>
        </div>
      )}

      {/*
        Pan overlay — only shown when zoomed in.
        Sits BELOW panels (zIndex 1) so panels stay interactive.
        Covers the whole screen to capture drag-to-pan on any empty area.
      */}
      {/* Canvas — pan by dragging on the canvas background */}
      <div
        className={styles.canvas}
        onDrop={handleDrop}
        onDragOver={handleDragOver}
        onMouseDown={(e) => {
          if (!isZoomedIn) return;
          if (e.target !== e.currentTarget) return; // ignore clicks on elements
          console.log("canvas mousedown");
          isPanning.current = true;
          panStart.current = { x: e.clientX, y: e.clientY };
          panOrigin.current = { x: pan.x, y: pan.y };
        }}
        onMouseMove={(e) => {
          if (!isPanning.current) return;
          console.log("panning");
          setPan({
            x: panOrigin.current.x + e.clientX - panStart.current.x,
            y: panOrigin.current.y + e.clientY - panStart.current.y,
          });
        }}
        onMouseUp={() => { isPanning.current = false; }}
        onMouseLeave={() => { isPanning.current = false; }}
        style={{
          width: props.canvasDimensions[0] * scale,
          height: props.canvasDimensions[1] * scale,
          cursor: isZoomedIn ? "grab" : "default",
          zIndex: 2,
        }}
      >
        {configState.elements.map(createJSXElement)}
      </div>
    </>
  );
}

export default Canvas;