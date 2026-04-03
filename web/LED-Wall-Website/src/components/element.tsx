import React, { useState, useEffect } from "react";
import { useDispatch } from "react-redux";
import {
  setSelectedElement,
  updateElement,
} from "../state/config/configSlice.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import ContextMenu from "./ContextMenu.tsx";
import useContextMenu from "../hooks/useContextMenu.tsx";
import { type Option } from "./ContextMenu.tsx";
import { clearElement } from "../state/config/configSlice.ts";


type ImageProps = {
  name: string;
  id: number;
  type: "image";
  path: string;
  location: [number, number];
  sizeMultiplier: number;
  scale: number;
  boxSizing?: string;
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
  boxSizing?: string;
};
<<<<<<< HEAD
type ElementProps = ImageProps | TextProps;
=======
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
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)

//Element that can be dragged and dropped inside the canvas
function Element(props: ElementProps) {
  //Redux State
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  //Store current position
  const [x, setX] = useState(0);
  const [y, setY] = useState(0);
  //Store position at start of dragging
  const [startX, setStartX] = useState(0);
  const [startY, setStartY] = useState(0);
  const [isDragging, setIsDragging] = useState(false);
  //Store current dimensions
  const [dimensions, setDimensions] = useState([0, 0]);
  //Store is font loaded
  const [fontLoaded, setFontLoaded] = useState(false);

  // ── Context menu ────────────────────────────────────────────────────────────
  const {
    location: contextLocation,
    setLocation: setContextLocation,
    isClicked: contextIsClicked,
    setIsClicked: setContextIsClicked,
  } = useContextMenu();

  const deleteOptions: Option[] = [{ name: "delete", function: deleteElement }];
  const [contextOptions, setContextOptions] = useState<Option[]>(deleteOptions);


  //Overwrites redux state of this element in the config
  function updateState() {
<<<<<<< HEAD
    if (props.type === "image") {
      dispatch(
        updateElement({
          name: props.name,
          id: props.id,
          type: "image",
          filepath: props.path,
          location: [props.location[0] + x, props.location[1] + y],
          scale: props.scale,
        })
      );
    } else if (props.type === "text") {
      dispatch(
        updateElement({
          name: props.name,
          id: props.id,
          type: "text",
          content: props.content,
          size: props.size,
          color: props.color,
          font_path: props.font_path,
          location: [props.location[0] + x, props.location[1] + y],
        })
      );
    }
  }

  function handleClick(e: React.MouseEvent) {
    //differentiate between left click (to drag) and riht click (to open context menu)
    if (e.button === 2) {
      //this is a right click, open context menu
      //"onContextMenu" prevents the default browser context menu from appearing
      setContextOptions(deleteOptions);
      setContextLocation([e.clientX - 380, e.clientY - 60]);
      setContextIsClicked(true);
    }
    else{
      //this is dragging
      dispatch(setSelectedElement(props.id));
      setIsDragging(true);
      setStartX(e.clientX - x);
      setStartY(e.clientY - y);
    }
  }
  

  function deleteElement() {
      dispatch(clearElement(configState.selectedElement));
=======
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
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
  }

  //Sends the current location of the element to the server
  function sendPosition() {
    fetch("/api/send-location", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        id: String(props.id),
        x: Math.trunc((props.location[0] + x) / props.sizeMultiplier),
        y: Math.trunc((props.location[1] + y) / props.sizeMultiplier),
      }),
    });
  }

  //Finds the size of the image and sets the new size with sizeMultiplier
  function handleLoad(e: React.SyntheticEvent<HTMLImageElement, Event>) {
    const { naturalHeight, naturalWidth } = e.currentTarget;
    setDimensions([
      naturalWidth * props.sizeMultiplier,
      naturalHeight * props.sizeMultiplier,
    ]);
  }

  //Had to change DragEvent to MouseEvent in order to have control over cursor style
  //Not using react's built in drag event required useEffect and event listeners,
  //because otherwise, the drag would stop if the cursor outpaced the image
  //Binding to the document solves that issue
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

  // Load font when component mounts or font_path changes
  useEffect(() => {
    if (props.type === "text" && props.font_path) {
      setFontLoaded(false);
      const fontFileName = props.font_path.split("/").pop() || "";
      const fontUrl = `/api/fonts/${fontFileName}`;
      const fontFamilyName = `customFont${props.id}`;

      // Create a new FontFace and load it
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
    if (props.type === "image") {
      return (
    <div
      onContextMenu={(e) => e.preventDefault()}
    >
      <img
        src={"/api/" + props.path}
        draggable={false}
        onMouseDown={(e) => handleClick(e)}
        onLoad={handleLoad}
        style={{
          position: "fixed",
          left: props.location[0] + x,
          top: props.location[1] + y,
          cursor: isDragging ? "grabbing" : "grab",
          width: dimensions[0] * props.scale,
          height: dimensions[1] * props.scale,
          margin: "-3px",
          boxSizing: "border-box",
          border:
            configState.selectedElement == props.id
              ? "3px solid cornflowerblue"
              : "3px solid transparent",
        }}
      />
      {contextIsClicked && (
        <ContextMenu options={contextOptions} location={contextLocation} />
      )}
    </div>
  );
    } else if (props.type === "text") {
      return (
        <div
          draggable={false}
          onMouseDown={(e) => handleClick(e)}
          onContextMenu={(e) => e.preventDefault()}
          style={{
            position: "fixed",
            left: props.location[0] + x,
            top: props.location[1] + y,
            cursor: isDragging ? "grabbing" : "grab",
            margin: "-3px",
            boxSizing: "border-box",
            border:
              configState.selectedElement == props.id
                ? "3px solid cornflowerblue"
                : "3px solid transparent",
          }}
        >
          <p
            style={{
              color: props.color,
              fontSize: props.size * props.sizeMultiplier,
              userSelect: "none",
              fontFamily: `customFont${props.id}, sans-serif`,
              visibility: fontLoaded ? "visible" : "hidden",
              margin: 0,
            }}
          >
            {props.content}
          </p>
<<<<<<< HEAD

          {contextIsClicked && (
          <ContextMenu options={contextOptions} location={contextLocation} />
          )}
=======
        </div>
      );
    } else {
      // Placeholder for carousel, video, webcam, rtmp
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
      : 64 * props.zoomScale;      const color = placeholderColors[props.type] ?? "#888";
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
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
        </div>
      );
    }
  }

  return <>{createJSXElement()}</>;
}
export default Element;
