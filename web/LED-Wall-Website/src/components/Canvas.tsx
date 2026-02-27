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
  const isPanning = useRef(false);
  const panStart = useRef({ x: 0, y: 0 });
  const panOrigin = useRef({ x: 0, y: 0 });

  const isZoomedIn = scale > 1;

  function handleZoomIn() {
    setScale(ZOOM_IN_SCALE);
    setPan({ x: 0, y: 0 });
  }

  function handleZoomOut() {
    setScale(1);
    setPan({ x: 0, y: 0 });
  }

  // Only pan when the user clicks directly on the canvas background,
  // not when clicking on a child element (image/text).
  function handleMouseDown(e: React.MouseEvent<HTMLDivElement>) {
    if (!isZoomedIn) return;
    // e.target is the exact element clicked; e.currentTarget is this div.
    // If they differ, the click landed on a child (an Element), so don't pan.
    if (e.target !== e.currentTarget) return;
    if (e.button !== 0) return;
    isPanning.current = true;
    panStart.current = { x: e.clientX, y: e.clientY };
    panOrigin.current = { x: pan.x, y: pan.y };
  }

  function handleMouseMove(e: React.MouseEvent<HTMLDivElement>) {
    if (!isPanning.current) return;
    const dx = e.clientX - panStart.current.x;
    const dy = e.clientY - panStart.current.y;
    setPan({ x: panOrigin.current.x + dx, y: panOrigin.current.y + dy });
  }

  function handleMouseUp() {
    isPanning.current = false;
  }

  function handleMouseLeave() {
    isPanning.current = false;
  }

  async function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    if (!e.dataTransfer.files || e.dataTransfer.files.length === 0) return;
    const canvasRect = e.currentTarget.getBoundingClientRect();
    const relativeX = Math.trunc((e.clientX - canvasRect.left) / props.sizeMultiplier);
    const relativeY = Math.trunc((e.clientY - canvasRect.top) / props.sizeMultiplier);
    uploadFile([relativeX, relativeY], e.dataTransfer.files[0], dispatch, configState);
  }

  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
  }

  function createJSXElement(element: Elem) {
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
          scale={element.scale}
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
        />
      );
    }
  }

  return (
    // Outer viewport — fixed size, clips overflow, holds border
    <div
      style={{
        position: "fixed",
        top: "53%",
        left: "50%",
        transform: "translate(-50%, -50%)",
        width: props.canvasDimensions[0],
        height: props.canvasDimensions[1],
        border: "4px solid rgb(29, 41, 58)",
        overflow: "hidden",
      }}
    >
      {/* Zoom buttons — sit above the canvas, stopPropagation so they don't trigger pan */}
      {props.canvasDimensions[1] > 0 && (
        <div
          style={{
            position: "absolute",
            bottom: "10px",
            left: "10px",
            display: "flex",
            alignItems: "center",
            gap: "6px",
            zIndex: 200,
            backgroundColor: "rgba(29,41,58,0.75)",
            borderRadius: "6px",
            padding: "4px 8px",
          }}
          onMouseDown={(e) => e.stopPropagation()}
        >
          <button
            onClick={handleZoomOut}
            disabled={!isZoomedIn}
            style={{
              left: "unset",
              transform: "none",
              margin: 0,
              padding: "2px 10px",
              opacity: isZoomedIn ? 1 : 0.45,
            }}
            title="Fit to screen"
          >
            −
          </button>
          <span style={{ color: "whitesmoke", fontSize: "13px", minWidth: "36px", textAlign: "center", userSelect: "none" }}>
            {isZoomedIn ? `Zoom: ${ZOOM_IN_SCALE}×` : "Zoom: 1×"}
          </span>
          <button
            onClick={handleZoomIn}
            disabled={isZoomedIn}
            style={{
              left: "unset",
              transform: "none",
              margin: 0,
              padding: "2px 10px",
              opacity: isZoomedIn ? 0.45 : 1,
            }}
            title="Zoom in"
          >
            +
          </button>
        </div>
      )}

      {/* Inner canvas — receives scale+pan transform, listens for background pan drags */}
      <div
        className={styles.canvas}
        onDrop={handleDrop}
        onDragOver={handleDragOver}
        onMouseDown={handleMouseDown}
        onMouseMove={handleMouseMove}
        onMouseUp={handleMouseUp}
        onMouseLeave={handleMouseLeave}
        style={{
          position: "absolute",
          top: 0,
          left: 0,
          border: "none",
          cursor: isZoomedIn ? "grab" : "default",
          width: props.canvasDimensions[0],
          height: props.canvasDimensions[1],
          transform: `translate(${pan.x}px, ${pan.y}px) scale(${scale})`,
          transformOrigin: "top left",
          transition: isPanning.current ? "none" : "transform 200ms ease",
        }}
      >
        {configState.elements.map(createJSXElement)}
      </div>
    </div>
  );
}

export default Canvas;