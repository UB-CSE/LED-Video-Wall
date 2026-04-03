import { useEffect, useState, type ChangeEvent } from "react";
import { useDispatch, useSelector } from "react-redux";
import type { RootState } from "../state/store";
import {reorderElements, setSelectedElement } from "../state/config/configSlice.ts";import styles from "../Styles.module.css";

// Move constants outside to prevent re-creation and ensure consistency
const INITIAL_COLOR = "#0025ff";
const INITIAL_SIZE = 24;

type Props = {
  sizeMultiplier: number;
  setAddTextIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddTextPopup({ setAddTextIsClicked }: Props) {
  const [name, setName] = useState("");
  const [content, setContent] = useState("");
  const [color, setColor] = useState(INITIAL_COLOR);
  const [fontSize, setFontSize] = useState(INITIAL_SIZE);
  const [fonts, setFonts] = useState<string[]>([]);

  const dispatch = useDispatch();
  // Only select what you need from the state to prevent unnecessary re-renders
  const elementsCount = useSelector((state: RootState) => state.config.elements.length);

  const handleClose = () => {
    setName("");
    setContent("");
    setColor(INITIAL_COLOR);
    setFontSize(INITIAL_SIZE);
    setAddTextIsClicked(false);
  };
  const configState = useSelector((state: RootState) => state.config);

  function handleAdd() {
    const newId = configState.elements.length + 1;
    const newElement = {
      name: name || `text${newId}`,
      id: 1,
      type: "text" as const,
      location: [0, 0],
      content,
      size: fontSize,
      color,
      font_path: fonts[0] ?? "",
      visible: true,
    };
    const shifted = configState.elements.map((el) => ({ ...el, id: el.id + 1 }));
    dispatch(reorderElements([newElement, ...shifted]));
    dispatch(setSelectedElement(1));
    handleClose();
  }

  useEffect(() => {
    let isMounted = true; // Guard to prevent state updates after unmount

    async function fetchFonts() {
      try {
        const response = await fetch("/api/list-fonts");
        const data = await response.json();
        if (isMounted && data.fonts) {
          setFonts(data.fonts);
        }
      } catch (error) {
        console.error("Could not fetch fonts", error);
      }
    }

    fetchFonts();
    return () => { isMounted = false; }; // Cleanup function
  }, []);

  return (
    <div className={styles.popup} onClick={(e) => e.stopPropagation()}>
      <textarea
        className={styles.box}
        value={content}
        onChange={(e) => setContent(e.target.value)}
        style={{ width: "80%", height: "150px", color, fontSize: `${fontSize}px` }}
      />
      
      <ul style={{ padding: 0, width: "80%", listStyle: "none" }}>
        <li style={{ display: "flex", marginBottom: "8px" }}>
          <span className={styles.label} style={{ width: "25%" }}>Name</span>
          <input
            className={styles.box}
            style={{ width: "75%" }}
            onChange={(e) => setName(e.target.value)}
            type="text"
            value={name}
            placeholder="Enter element name..."
          />
        </li>
        <li style={{ display: "flex" }}>
          <span className={styles.label} style={{ width: "25%" }}>Font Size</span>
          <input
            className={styles.box}
            style={{ width: "75%" }}
            onChange={(e) => setFontSize(e.target.valueAsNumber || 0)}
            type="number"
            value={fontSize}
          />
        </li>
      </ul>

      <div className={styles.buttonGroup}>
        <button onClick={handleClose}>Cancel</button>
        <button onClick={handleAdd} disabled={!content.trim()}>Add</button>
      </div>
    </div>
  );
}

export default AddTextPopup;