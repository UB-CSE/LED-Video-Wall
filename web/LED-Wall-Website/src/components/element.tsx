import React, { useState, useEffect, useContext } from "react";
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
import PortsContext from "../PortContext";


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
type ElementProps = ImageProps | TextProps;

//Element that can be dragged and dropped inside the canvas
function Element(props: ElementProps) {
  //Redux State
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();
  const ports = useContext(PortsContext) as { ledvwPort?: number } | undefined;
  const ledvwPort = ports?.ledvwPort ?? 7070;

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
  }

  //Sends the current location of the element to the server
  function sendPosition() {
    fetch(`/api/${ledvwPort}/send-location`, {
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

          {contextIsClicked && (
          <ContextMenu options={contextOptions} location={contextLocation} />
          )}
        </div>
      );
    }
  }

  return <>{createJSXElement()}</>;
}
export default Element;
