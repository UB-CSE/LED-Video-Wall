import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useDispatch } from "react-redux";
import { setSelectedElement, updateElement } from "../state/config/configSlice.ts";
import type React from "react";
import { useState } from "react";
import ContextMenu from "./ContextMenu.tsx";
import useContextMenu from "../hooks/useContextMenu.tsx";
import { type Option } from "./ContextMenu.tsx";
import AddImagePopup from "./AddImagePopup.tsx";
import AddTextPopup from "./AddTextPopup.tsx";

type Props = {
  sizeMultiplier: number;
};

// List of all elements in the current configuration with ability to add elements.
// Elements are ordered back-to-front: index 0 = back (id 1), last index = front (highest id).
function ElementList(props: Props) {
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  // ── Context menu ────────────────────────────────────────────────────────────
  const {
    location: contextLocation,
    setLocation: setContextLocation,
    isClicked: contextIsClicked,
    setIsClicked: setContextIsClicked,
  } = useContextMenu();

  const { isClicked: addImageIsClicked, setIsClicked: setAddImageIsClicked } =
    useContextMenu();
  const { isClicked: addTextIsClicked, setIsClicked: setAddTextIsClicked } =
    useContextMenu();

  // ── Drag-and-drop state ──────────────────────────────────────────────────────
  // draggedId  – the element.id of the item being dragged
  // dragOverId – the element.id of the slot currently being hovered over
  const [draggedId, setDraggedId] = useState<number | null>(null);
  const [dragOverId, setDragOverId] = useState<number | null>(null);

  function handleDragStart(e: React.DragEvent<HTMLLIElement>, id: number) {
    setDraggedId(id);
    dispatch(setSelectedElement(id)); // select the item being dragged immediately
    e.dataTransfer.effectAllowed = "move";
  }

  function handleDragOver(e: React.DragEvent<HTMLLIElement>, id: number) {
    e.preventDefault();
    e.dataTransfer.dropEffect = "move";
    if (id !== dragOverId) setDragOverId(id);
  }

  function handleDragLeave() {
    setDragOverId(null);
  }

  function handleDragEnd() {
    setDraggedId(null);
    setDragOverId(null);
  }

  // Reorders layers so that the dragged element lands at the position occupied
  // by the target element. Mirrors the ID-reassignment logic in DetailsPanel's
  // handleLayerChange so both paths stay in sync.
  async function handleDrop(e: React.DragEvent<HTMLLIElement>, targetId: number) {
    e.preventDefault();
    setDragOverId(null);

    if (draggedId === null || draggedId === targetId) {
      setDraggedId(null);
      return;
    }

    // Work on a stable copy sorted by current id (back → front)
    const sorted = [...configState.elements].sort((a, b) => a.id - b.id);
    const draggedIndex = sorted.findIndex((el) => el.id === draggedId);
    const targetIndex = sorted.findIndex((el) => el.id === targetId);

    if (draggedIndex === -1 || targetIndex === -1) {
      setDraggedId(null);
      return;
    }

    // Reorder: remove dragged item, insert at target position
    const reordered = [...sorted];
    const [draggedItem] = reordered.splice(draggedIndex, 1);
    reordered.splice(targetIndex, 0, draggedItem);

    // Reassign ids 1…n to match new visual order, then dispatch to Redux
    reordered.forEach((el, index) => {
      const newId = index + 1;
      if (el.id !== newId) {
        dispatch(updateElement({ ...el, id: newId }));
      }
    });

    // Keep the dragged element selected at its new id —
    // derive from reordered array so it's always correct regardless of direction
    const newId = reordered.findIndex((el) => el.name === draggedItem.name) + 1;
    dispatch(setSelectedElement(newId));
    setDraggedId(null);

    // Persist the new order to the backend (fire-and-forget; no reload needed)
    const newOrder = reordered.map((el) => el.name);
    try {
      const response = await fetch("/api/reorder-layers", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ layer_list: newOrder }),
      });
      if (!response.ok) {
        console.error("Failed to persist layer order:", await response.text());
      }
    } catch (error) {
      console.error("Failed to reach reorder-layers endpoint:", error);
    }
  }

  // ── Context menu options ─────────────────────────────────────────────────────
  const deleteOptions: Option[] = [{ name: "delete", function: deleteElement }];
  const addOptions: Option[] = [
    { name: "image", function: addImage },
    { name: "text", function: addText },
  ];
  const [contextOptions, setContextOptions] = useState<Option[]>(deleteOptions);

  function handleClick(id: number) {
    dispatch(setSelectedElement(id));
  }

  function handleRightClick(e: React.MouseEvent<HTMLLIElement>) {
    setContextOptions(deleteOptions);
    e.preventDefault();
    setContextLocation([e.clientX, e.clientY]);
    setContextIsClicked(true);
  }

  function handleAdd(e: React.MouseEvent) {
    setContextOptions(addOptions);
    e.preventDefault();
    e.stopPropagation();
    setContextLocation([e.clientX, e.clientY]);
    setContextIsClicked(true);
  }

  function deleteElement() {}

  function addImage(e: React.MouseEvent) {
    setAddImageIsClicked(true);
    e.preventDefault();
    e.stopPropagation();
  }

  function addText(e: React.MouseEvent) {
    setAddTextIsClicked(true);
    e.preventDefault();
    e.stopPropagation();
  }

  // Render sorted back→front so the visual order matches the layer stack
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
        <h3 style={{ marginLeft: "178px" }}>type</h3>
      </header>
      <div style={{ width: "100%", height: "100%", overflowY: "scroll" }}>
        <ul style={{ paddingLeft: "0px" }}>
          {sortedElements.map((element) => {
            const isSelected = configState.selectedElement === element.id;
            const isBeingDragged = draggedId === element.id;
            const isDropTarget = dragOverId === element.id && !isBeingDragged;

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
                onContextMenu={(e) => handleRightClick(e)}
                style={{
                  display: "flex",
                  cursor: "grab",
                  opacity: isBeingDragged ? 0.4 : 1,
                  // Blue selection border takes priority; drop-target gets a
                  // dashed orange indicator so the user can see where it'll land
                  border: "2px solid",
                  borderColor: isSelected
                    ? "cornflowerblue"
                    : isDropTarget
                    ? "orange"
                    : "transparent",
                  borderStyle: isDropTarget ? "dashed" : "solid",
                  backgroundColor: isDropTarget ? "rgba(255,165,0,0.12)" : "transparent",
                  transition: "border-color 80ms ease, background-color 80ms ease",
                  boxSizing: "border-box",
                }}
              >
                {/* Drag handle hint */}
                <span
                  style={{
                    display: "flex",
                    alignItems: "center",
                    padding: "0 4px",
                    color: "#aaa",
                    fontSize: "12px",
                    userSelect: "none",
                    letterSpacing: "-1px",
                  }}
                  title="Drag to reorder"
                >
                  ⠿
                </span>
                <p className={styles.box} style={{ width: "15%" }}>
                  {element.id}
                </p>
                <p className={styles.box} style={{ width: "55%" }}>
                  {element.name}
                </p>
                <p className={styles.box} style={{ width: "30%" }}>
                  {element.type}
                </p>
              </li>
            );
          })}
        </ul>
      </div>
      <header style={{ display: "flex" }}>
        <h3>front</h3>
      </header>
      {contextIsClicked && (
        <ContextMenu options={contextOptions} location={contextLocation} />
      )}
      {addImageIsClicked && (
        <AddImagePopup
          sizeMultiplier={props.sizeMultiplier}
          setAddImageIsClicked={setAddImageIsClicked}
        />
      )}
      {addTextIsClicked && (
        <AddTextPopup
          sizeMultiplier={props.sizeMultiplier}
          setAddTextIsClicked={setAddTextIsClicked}
        />
      )}
    </div>
  );
}

export default ElementList;