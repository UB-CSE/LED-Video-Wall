import styles from "../Styles.module.css";
import { useEffect, useState, type ChangeEvent } from "react";
import { useDispatch } from "react-redux";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { addElement } from "../state/config/configSlice.ts";

type Props = {
  sizeMultiplier: number;
  setAddTextIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddTextPopup(props: Props) {
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);
  const [name, setName] = useState("");
  const [content, setContent] = useState("");
  const [color, setColor] = useState("#000000ff");
  const [fontSize, setFontSize] = useState(24);
  const [fonts, setFonts] = useState<string[]>([]);

  function handleClose() {
    setName("");
    setContent("");
    setColor("#000000ff");
    setFontSize(24);
    props.setAddTextIsClicked(false);
  }

  function handleChange(e: ChangeEvent<HTMLTextAreaElement>) {
    setContent(e.target.value);
  }

  function handleAdd() {
    dispatch(
      addElement({
        name: name,
        id: configState.elements.length + 1,
        type: "text",
        location: [0, 0],
        content: content,
        size: fontSize,
        color: color,
        font_path: fonts[0],
      })
    );
    handleClose();
  }

  //Fetch list of available fonts on component mount
  useEffect(() => {
    async function fetchFonts() {
      try {
        const response = await fetch("/api/list-fonts", { method: "GET" });
        const data = await response.json();
        if (data.fonts) {
          setFonts(data.fonts);
        } else if (data.error) {
          console.log(`[ERROR]: ${data.error}`);
        }
      } catch (error) {
        console.log("[ERROR]: Could not fetch fonts");
      }
    }
    fetchFonts();
  }, []);

  return (
    <div
      className={styles.popup}
      onClick={(e) => {
        e.stopPropagation();
      }}
    >
      <textarea
        className={styles.box}
        value={content}
        onChange={(e) => handleChange(e)}
        style={{
          width: "80%",
          height: "150px",
          color: color,
          fontSize: fontSize,
        }}
      />
      <ul style={{ padding: "0px", width: "80%" }}>
        <li key={1} style={{ display: "flex" }}>
          <p className={styles.box} style={{ width: "25%" }}>
            name
          </p>
          <input
            className={styles.box}
            style={{ width: "75%" }}
            onChange={(e) => setName(e.target.value)}
            type="text"
            value={name}
          />
        </li>
        <li
          key={2}
          style={{
            display: "flex",
          }}
        >
          <p
            className={styles.box}
            style={{
              width: "25%",
            }}
          >
            font size
          </p>
          <input
            className={styles.box}
            style={{ width: "75%" }}
            onChange={(e) => {
              setFontSize(e.target.valueAsNumber);
            }}
            type="number"
            value={fontSize}
          />
        </li>
      </ul>
      <button
        onClick={handleAdd}
        style={{ left: "0%", transform: "translate(0%, 0%)" }}
      >
        Add
      </button>
    </div>
  );
}

export default AddTextPopup;
