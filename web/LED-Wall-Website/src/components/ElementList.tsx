import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useDispatch } from "react-redux";
import {
  setSelectedElement,
  updateElement,
  addElement,
  toggleElementVisibility,
} from "../state/config/configSlice.ts";
import type React from "react";
import { useState } from "react";
import ContextMenu from "./ContextMenu.tsx";
import useContextMenu from "../hooks/useContextMenu.tsx";
import { type Option } from "./ContextMenu.tsx";
import AddImagePopup from "./AddImagePopup.tsx";
import AddTextPopup from "./AddTextPopup.tsx";
import AddCarouselPopup from "./AddCarouselPopup.tsx";
import AddVideoPopup from "./AddVideoPopup.tsx";
import AddWebcamPopup from "./AddWebcamPopup.tsx";
import AddRtmpPopup from "./AddRtmpPopup.tsx";

type Props = {
  sizeMultiplier: number;
};

function EyeIcon({ visible }: { visible: boolean }) {
  return visible ? (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z" />
      <circle cx="12" cy="12" r="3" />
    </svg>
  ) : (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <line x1="1" y1="1" x2="23" y2="23" />
      <path d="M17.94 17.94A10.07 10.07 0 0112 20c-7 0-11-8-11-8a18.45 18.45 0 015.06-5.94" />
      <path d="M9.9 4.24A9.12 9.12 0 0112 4c7 0 11 8 11 8a18.5 18.5 0 01-2.16 3.19" />
    </svg>
  );
}

function ElementList(props: Props) {
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  // ── Context menu ──────────────────────────────────────────────────────────
  const { location: contextLocation, setLocation: setContextLocation, isClicked: contextIsClicked, setIsClicked: setContextIsClicked } = useContextMenu();
  const { isClicked: addImageIsClicked, setIsClicked: setAddImageIsClicked } = useContextMenu();
  const { isClicked: addTextIsClicked, setIsClicked: setAddTextIsClicked } = useContextMenu();
  const { isClicked: addCarouselIsClicked, setIsClicked: setAddCarouselIsClicked } = useContextMenu();
  const { isClicked: addVideoIsClicked, setIsClicked: setAddVideoIsClicked } = useContextMenu();
  const { isClicked: addWebcamIsClicked, setIsClicked: setAddWebcamIsClicked } = useContextMenu();
  const { isClicked: addRtmpIsClicked, setIsClicked: setAddRtmpIsClicked } = useContextMenu();
  const [contextElementId, setContextElementId] = useState<number | null>(null);
  const [contextOptions, setContextOptions] = useState<Option[]>([]);

  // ── Drag-to-reorder ───────────────────────────────────────────────────────
  const [draggedId, setDraggedId] = useState<number | null>(null);
  const [dragOverId, setDragOverId] = useState<number | null>(null);

  function handleDragStart(e: React.DragEvent<HTMLLIElement>, id: number) {
    setDraggedId(id);
    dispatch(setSelectedElement(id));
    e.dataTransfer.effectAllowed = "move";
  }

  function handleDragOver(e: React.DragEvent<HTMLLIElement>, id: number) {
    e.preventDefault();
    e.dataTransfer.dropEffect = "move";
    if (id !== dragOverId) setDragOverId(id);
  }

  function handleDragLeave() { setDragOverId(null); }
  function handleDragEnd() { setDraggedId(null); setDragOverId(null); }

  async function handleDrop(e: React.DragEvent<HTMLLIElement>, targetId: number) {
    e.preventDefault();
    setDragOverId(null);
    if (draggedId === null || draggedId === targetId) { setDraggedId(null); return; }

    const sorted = [...configState.elements].sort((a, b) => a.id - b.id);
    const draggedIndex = sorted.findIndex((el) => el.id === draggedId);
    const targetIndex = sorted.findIndex((el) => el.id === targetId);
    if (draggedIndex === -1 || targetIndex === -1) { setDraggedId(null); return; }

    const reordered = [...sorted];
    const [draggedItem] = reordered.splice(draggedIndex, 1);
    reordered.splice(targetIndex, 0, draggedItem);

    reordered.forEach((el, index) => {
      const newId = index + 1;
      if (el.id !== newId) dispatch(updateElement({ ...el, id: newId }));
    });

    const newId = reordered.findIndex((el) => el.name === draggedItem.name) + 1;
    dispatch(setSelectedElement(newId));
    setDraggedId(null);

    try {
      await fetch("/api/reorder-layers", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ layer_list: reordered.map((el) => el.name) }),
      });
    } catch (error) {
      console.error("Failed to reach reorder-layers endpoint:", error);
    }
  }

  // ── Visibility (UI only, resets on refresh) ───────────────────────────────
  function handleToggleVisibility(e: React.MouseEvent, id: number) {
    e.stopPropagation();
    dispatch(toggleElementVisibility(id));
  }

  // ── Duplicate ─────────────────────────────────────────────────────────────
  async function duplicateElement() {
    if (contextElementId === null) return;
    const element = configState.elements.find((el) => el.id === contextElementId);
    if (!element) return;

    try {
      const response = await fetch("/api/duplicate-layer", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name: element.name }),
      });
      if (!response.ok) { console.error("Failed to duplicate layer"); return; }
      const data = await response.json();

      const newId = configState.elements.length + 1;
      if (element.type === "image") {
        dispatch(addElement({
          name: data.new_name,
          id: newId,
          type: "image",
          filepath: element.filepath,
          location: [element.location[0] + props.sizeMultiplier, element.location[1] + props.sizeMultiplier],
          scale: element.scale,
          visible: true,
        }));
      } else if (element.type === "text") {
        dispatch(addElement({
          name: data.new_name,
          id: newId,
          type: "text",
          content: element.content,
          size: element.size,
          color: element.color,
          font_path: element.font_path,
          location: [element.location[0] + props.sizeMultiplier, element.location[1] + props.sizeMultiplier],
          visible: true,
        }));
      }
      dispatch(setSelectedElement(newId));
    } catch (error) {
      console.error("Failed to duplicate layer:", error);
    }
  }

  // ── Delete (pre-existing stub) ────────────────────────────────────────────
  function deleteElement() {}

  // ── Add ───────────────────────────────────────────────────────────────────
  function addImage(e: React.MouseEvent) { setAddImageIsClicked(true); e.preventDefault(); e.stopPropagation(); }
  function addText(e: React.MouseEvent) { setAddTextIsClicked(true); e.preventDefault(); e.stopPropagation(); }
  function addCarousel(e: React.MouseEvent) { setAddCarouselIsClicked(true); e.preventDefault(); e.stopPropagation(); }
  function addVideo(e: React.MouseEvent) { setAddVideoIsClicked(true); e.preventDefault(); e.stopPropagation(); }
  function addWebcam(e: React.MouseEvent) { setAddWebcamIsClicked(true); e.preventDefault(); e.stopPropagation(); }
  function addRtmp(e: React.MouseEvent) { setAddRtmpIsClicked(true); e.preventDefault(); e.stopPropagation(); }

  function handleClick(id: number) { dispatch(setSelectedElement(id)); }

  function handleRightClick(e: React.MouseEvent<HTMLLIElement>, id: number) {
    setContextElementId(id);
    setContextOptions([
      { name: "duplicate", function: duplicateElement },
      { name: "delete", function: deleteElement },
    ]);
    e.preventDefault();
    setContextLocation([e.clientX, e.clientY]);
    setContextIsClicked(true);
  }

  function handleAdd(e: React.MouseEvent) {
    setContextOptions([
      { name: "image", function: addImage },
      { name: "text", function: addText },
      { name: "carousel", function: addCarousel },
      { name: "video", function: addVideo },
      { name: "webcam", function: addWebcam },
      { name: "rtmp", function: addRtmp },
    ]);
    e.preventDefault();
    e.stopPropagation();
    setContextLocation([e.clientX, e.clientY]);
    setContextIsClicked(true);
  }

  const sortedElements = [...configState.elements].sort((a, b) => a.id - b.id);

  return (
    <div className={styles.panel} style={{ height: "325px" }}>
      <div style={{ display: "flex", backgroundColor: "dimgrey" }}>
        <button onClick={(e) => handleAdd(e)} className={styles.addButton}>
          <span style={{ fontSize: "32px", marginTop: "-8px" }}>+</span>
        </button>
        <h2 className={styles.panelHeader}>Element List</h2>
      </div>
      <header style={{ display: "flex" }}>
        <h3>back</h3>
        <h3 style={{ marginLeft: "155px" }}>type</h3>
      </header>
      <div style={{ width: "100%", height: "100%", overflowY: "scroll" }}>
        <ul style={{ paddingLeft: "0px" }}>
          {sortedElements.map((element) => {
            const isSelected = configState.selectedElement === element.id;
            const isBeingDragged = draggedId === element.id;
            const isDropTarget = dragOverId === element.id && !isBeingDragged;
            const isVisible = element.visible !== false;

            return (
              <li
                key={element.id}
                draggable={true}
                onDragStart={(e) => handleDragStart(e, element.id)}
                onDragOver={(e) => handleDragOver(e, element.id)}
                onDragLeave={handleDragLeave}
                onDrop={(e) => handleDrop(e, element.id)}
                onDragEnd={handleDragEnd}
                onClick={() => handleClick(element.id)}
                onContextMenu={(e) => handleRightClick(e, element.id)}
                style={{
                  display: "flex",
                  alignItems: "center",
                  cursor: "grab",
                  opacity: isBeingDragged ? 0.4 : isVisible ? 1 : 0.45,
                  border: "2px solid",
                  borderColor: isSelected ? "cornflowerblue" : isDropTarget ? "orange" : "transparent",
                  borderStyle: isDropTarget ? "dashed" : "solid",
                  backgroundColor: isDropTarget ? "rgba(255,165,0,0.12)" : "transparent",
                  boxSizing: "border-box",
                }}
              >
                <span style={{ padding: "0 4px", color: "#aaa", fontSize: "12px", userSelect: "none", letterSpacing: "-1px" }}>⠿</span>
                <p className={styles.box} style={{ width: "15%" }}>{element.id}</p>
                <p className={styles.box} style={{ width: "42%", fontStyle: isVisible ? "normal" : "italic" }}>{element.name}</p>
                <p className={styles.box} style={{ width: "25%" }}>{element.type}</p>
                <button
                  onClick={(e) => handleToggleVisibility(e, element.id)}
                  title={isVisible ? "Hide layer" : "Show layer"}
                  style={{
                    width: "18%",
                    left: "unset",
                    transform: "none",
                    margin: 0,
                    padding: "4px",
                    border: "none",
                    boxShadow: "none",
                    backgroundColor: "transparent",
                    cursor: "pointer",
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                    color: isVisible ? "rgb(29,41,58)" : "#aaa",
                  }}
                >
                  <EyeIcon visible={isVisible} />
                </button>
              </li>
            );
          })}
        </ul>
      </div>
      <header style={{ display: "flex" }}>
        <h3>front</h3>
      </header>
      {contextIsClicked && <ContextMenu options={contextOptions} location={contextLocation} />}
      {addImageIsClicked && <AddImagePopup sizeMultiplier={props.sizeMultiplier} setAddImageIsClicked={setAddImageIsClicked} />}
      {addTextIsClicked && <AddTextPopup sizeMultiplier={props.sizeMultiplier} setAddTextIsClicked={setAddTextIsClicked} />}
      {addCarouselIsClicked && <AddCarouselPopup sizeMultiplier={props.sizeMultiplier} setAddCarouselIsClicked={setAddCarouselIsClicked} />}
      {addVideoIsClicked && <AddVideoPopup sizeMultiplier={props.sizeMultiplier} setAddVideoIsClicked={setAddVideoIsClicked} />}
      {addWebcamIsClicked && <AddWebcamPopup sizeMultiplier={props.sizeMultiplier} setAddWebcamIsClicked={setAddWebcamIsClicked} />}
      {addRtmpIsClicked && <AddRtmpPopup sizeMultiplier={props.sizeMultiplier} setAddRtmpIsClicked={setAddRtmpIsClicked} />}
    </div>
  );
}

export default ElementList;