import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useEffect, useState } from "react";
import { useDispatch } from "react-redux";
import { setSelectedElement, updateElement } from "../state/config/configSlice";

type Props = {
  sizeMultiplier: number;
};

function DetailsPanel(props: Props) {
  const configState = useSelector((state: RootState) => state.config);
  const [type, setType] = useState("");
  const [name, setName] = useState("");
  const [id, setId] = useState(0);
  const [path, setPath] = useState("");
  const [layer, setLayer] = useState(0);
  const [location, setLocation] = useState<number[]>([0, 0]);

  //image only
  const [scale, setScale] = useState(1);

  //text only
  const [fontSize, setFontSize] = useState(0);
  const [color, setColor] = useState("");
  const [content, setContent] = useState("");
  const [fonts, setFonts] = useState<string[]>([]);

  const dispatch = useDispatch();

  async function handleChange(e: React.KeyboardEvent<HTMLInputElement>) {
    if (scale < 0) {
      await setScale(0);
    }
    if (fontSize < 0) {
      await setFontSize(0);
    }
    if (e.key === "Enter") {
      if (type === "image") {
        dispatch(
          updateElement({
            name: name,
            id: id,
            type: "image",
            filepath: path,
            location: [
              location[0] * props.sizeMultiplier,
              location[1] * props.sizeMultiplier,
            ],
            scale: scale,
          })
        );
      } else if (type === "text") {
        dispatch(
          updateElement({
            name: name,
            id: id,
            type: "text",
            content: content,
            size: fontSize,
            color: color,
            font_path: path,
            location: [
              location[0] * props.sizeMultiplier,
              location[1] * props.sizeMultiplier,
            ],
          })
        );
      }
      //Send updated position to server
      fetch("/api/send-location", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          id: String(id),
          x: location[0],
          y: location[1],
        }),
      });
    }
  }

  async function handleLayerChange(e: React.KeyboardEvent<HTMLInputElement>) {
    if (e.key === "Enter") {
      const oldElements = configState.elements;

      //Clamp layer value to 1-length of elements and if the layer is the same as current, return early
      if (layer < 1) {
        await setLayer(1);
        return;
      }
      if (layer === id) {
        return;
      }
      if (layer > oldElements.length) {
        await setLayer(oldElements.length);
        return;
      }

      let newLayer = 1;
      for (let oldLayer = 1; oldLayer <= oldElements.length; oldLayer++) {
        // When we reach the desired layer, insert the moved element
        if (newLayer === layer) {
          if (type === "image") {
            dispatch(
              updateElement({
                name: name,
                id: newLayer,
                type: type,
                filepath: path,
                location: [
                  location[0] * props.sizeMultiplier,
                  location[1] * props.sizeMultiplier,
                ],
                scale: scale,
              })
            );
          } else if (type === "text") {
            dispatch(
              updateElement({
                name: name,
                id: newLayer,
                type: type,
                content: content,
                size: fontSize,
                color: color,
                font_path: path,
                location: [
                  location[0] * props.sizeMultiplier,
                  location[1] * props.sizeMultiplier,
                ],
              })
            );
          }
          newLayer++;
        }
        if (oldLayer === id) {
          continue;
        }
        const element = { ...oldElements[oldLayer - 1] };
        element.id = newLayer;
        dispatch(updateElement(element));
        newLayer++;
      }
      if (newLayer === layer) {
        if (type === "image") {
          dispatch(
            updateElement({
              name: name,
              id: newLayer,
              type: type,
              filepath: path,
              location: [
                location[0] * props.sizeMultiplier,
                location[1] * props.sizeMultiplier,
              ],
              scale: scale,
            })
          );
        } else if (type === "text") {
          dispatch(
            updateElement({
              name: name,
              id: newLayer,
              type: type,
              content: content,
              size: fontSize,
              color: color,
              font_path: path,
              location: [
                location[0] * props.sizeMultiplier,
                location[1] * props.sizeMultiplier,
              ],
            })
          );
        }
      }
      dispatch(setSelectedElement(layer));
      setId(layer);
    }
  }

  function handleFontChange(e: React.ChangeEvent<HTMLSelectElement>) {
    const path = e.target.value;
    setPath(path);
    dispatch(
      updateElement({
        name: name,
        id: id,
        type: "text",
        content: content,
        size: fontSize,
        color: color,
        font_path: path,
        location: [
          location[0] * props.sizeMultiplier,
          location[1] * props.sizeMultiplier,
        ],
      })
    );
  }

  useEffect(() => {
    const element = configState.elements[configState.selectedElement - 1];
    if (element) {
      setType(element.type);
      setName(element.name);
      setId(element.id);
      setLayer(element.id);
      setLocation([
        Math.trunc(element.location[0] / props.sizeMultiplier),
        Math.trunc(element.location[1] / props.sizeMultiplier),
      ]);
      if (element.type === "image") {
        setPath(element.filepath);
        setScale(element.scale);
      } else if (element.type === "text") {
        setPath(element.font_path);
        setFontSize(element.size);
        setColor(element.color);
        setContent(element.content);
      }
    } else {
      setType("");
    }
  }, [configState.selectedElement, configState.elements]);

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
    <div className={styles.panel}>
      <h2
        className={styles.panelHeader}
        style={{ paddingRight: "0px", marginBottom: "0px" }}
      >
        Details Panel
      </h2>
      <div style={{ width: "350px", height: "220px", overflowY: "scroll" }}>
        <ul
          style={{
            paddingLeft: "0px",
            marginTop: "0px",
          }}
        >
          {type && (
            <li
              key={1}
              style={{
                display: "flex",
              }}
            >
              <p className={styles.box} style={{ width: "25%" }}>
                type
              </p>
              <p className={styles.box} style={{ width: "75%" }}>
                {type}
              </p>
            </li>
          )}
          {type && (
            <li
              key={2}
              style={{
                display: "flex",
              }}
            >
              <p className={styles.box} style={{ width: "25%" }}>
                name
              </p>
              <input
                className={styles.box}
                style={{ width: "75%" }}
                onChange={(e) => setName(e.target.value)}
                onKeyDown={(e) => handleChange(e)}
                type="text"
                value={name}
              />
            </li>
          )}
          {type && (
            <li
              key={3}
              style={{
                display: "flex",
              }}
            >
              <p className={styles.box} style={{ width: "25%" }}>
                layer
              </p>
              <input
                className={styles.box}
                style={{ width: "75%" }}
                onChange={(e) => {
                  setLayer(e.target.valueAsNumber);
                }}
                onKeyDown={(e) => handleLayerChange(e)}
                type="number"
                value={layer}
              />
            </li>
          )}
          {type && (
            <li
              key={4}
              style={{
                display: "flex",
              }}
            >
              <p className={styles.box} style={{ width: "24.5%" }}>
                location
              </p>
              <div
                className={styles.box}
                style={{ width: "75.5%", display: "flex", padding: "3px" }}
              >
                <p>x:</p>
                <input
                  onChange={(e) => {
                    setLocation([e.target.valueAsNumber, location[1]]);
                  }}
                  onKeyDown={(e) => handleChange(e)}
                  type="number"
                  value={location[0]}
                  style={{
                    width: "20%",
                    backgroundColor: "whitesmoke",
                    margin: "auto",
                  }}
                />
                <p>y:</p>
                <input
                  onChange={(e) => {
                    setLocation([location[0], e.target.valueAsNumber]);
                  }}
                  onKeyDown={(e) => handleChange(e)}
                  type="number"
                  value={location[1]}
                  style={{
                    width: "20%",
                    backgroundColor: "whitesmoke",
                    margin: "auto",
                  }}
                />
              </div>
            </li>
          )}
          {type == "image" && (
            <li
              key={5}
              style={{
                display: "flex",
              }}
            >
              <p className={styles.box} style={{ width: "25%" }}>
                scale
              </p>
              <input
                className={styles.box}
                style={{ width: "75%" }}
                onChange={(e) => {
                  if (e.target.valueAsNumber >= 0) {
                    setScale(e.target.valueAsNumber);
                  } else {
                    setScale(0);
                  }
                }}
                onKeyDown={(e) => handleChange(e)}
                type="number"
                value={scale}
              />
            </li>
          )}
          {type == "text" && (
            <li
              key={5}
              style={{
                display: "flex",
              }}
            >
              <p className={styles.box} style={{ width: "25%" }}>
                content
              </p>
              <input
                className={styles.box}
                style={{ width: "75%" }}
                onChange={(e) => setContent(e.target.value)}
                onKeyDown={(e) => handleChange(e)}
                type="text"
                value={content}
              />
            </li>
          )}
          {type == "text" && (
            <li
              key={6}
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
                onKeyDown={(e) => handleChange(e)}
                type="number"
                value={fontSize}
              />
            </li>
          )}
          {type == "text" && (
            <li
              key={7}
              style={{
                display: "flex",
              }}
            >
              <p
                className={styles.box}
                style={{
                  width: "24%",
                }}
              >
                font
              </p>
              <select
                className={styles.box}
                value={path}
                onChange={handleFontChange}
                style={{
                  width: "76%",
                  margin: "0px",
                  position: "static",
                  left: "0",
                  transform: "none",
                  padding: "5px",
                  boxShadow: "none",
                }}
              >
                {fonts.map((font) => {
                  const fileName = font.split("/").pop() || font;
                  return (
                    <option key={font} value={font}>
                      {fileName}
                    </option>
                  );
                })}
              </select>
            </li>
          )}
          {type == "text" && (
            <li
              key={8}
              style={{
                display: "flex",
              }}
            >
              <p
                className={styles.box}
                style={{
                  width: "24%",
                }}
              >
                color
              </p>
              <input
                className={styles.box}
                style={{
                  width: "76%",
                  height: "40px",
                  margin: "0px",
                  position: "static",
                  left: "0",
                  transform: "none",
                  padding: "5px",
                  boxShadow: "none",
                }}
                onChange={(e) => setColor(e.target.value)}
                onKeyDown={(e) => handleChange(e)}
                type="color"
                value={color}
              />
            </li>
          )}
        </ul>
      </div>
    </div>
  );
}
export default DetailsPanel;
