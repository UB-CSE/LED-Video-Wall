import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useDispatch } from "react-redux";
import { setSelectedElement, updateElement } from "../state/config/configSlice";
import type { Elem } from "../state/config/configSlice";
import { useEffect, useState, useRef } from "react";



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

<<<<<<< HEAD
=======
  // carousel only
  const [filepaths, setFilepaths] = useState<string[]>([]);

  // shared: carousel, video, webcam, rtmp
  const [framerate, setFramerate] = useState(30);

  // webcam only
  const [cameraNumber, setCameraNumber] = useState(0);

  // rtmp only
  const [streamName, setStreamName] = useState("");
  const [rtmpSize, setRtmpSize] = useState<number[]>([0, 0]);
  

>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
  const dispatch = useDispatch();
  const rtmpSizeRef = useRef<number[]>([0, 0]);


  async function handleChange() {
    if (scale < 0) {
      await setScale(0);
    }
    if (fontSize < 0) {
      await setFontSize(0);
    }
<<<<<<< HEAD
=======
    if (e.key === "Enter") {
      console.log("handleChange fired, type:", type); 
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
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
<<<<<<< HEAD
        );
      }
=======
        ); } else if (type === "rtmp") {
          console.log("inside rtmp block, rtmpSizeRef:", rtmpSizeRef.current);
          const size = rtmpSizeRef.current[0] > 0 || rtmpSizeRef.current[1] > 0 ? rtmpSizeRef.current : undefined;          console.log("size value:", size);
          dispatch(updateElement({ name, id, type: "rtmp", location: [location[0] * props.sizeMultiplier, location[1] * props.sizeMultiplier], stream_name: streamName, framerate, size }));
        }else if (type === "carousel") {
          dispatch(updateElement({ name, id, type: "carousel", location: [location[0] * props.sizeMultiplier, location[1] * props.sizeMultiplier], filepaths, framerate }));
        } else if (type === "video") {
          dispatch(updateElement({ name, id, type: "video", location: [location[0] * props.sizeMultiplier, location[1] * props.sizeMultiplier], filepath: path, framerate }));
        } else if (type === "webcam") {
          dispatch(updateElement({ name, id, type: "webcam", location: [location[0] * props.sizeMultiplier, location[1] * props.sizeMultiplier], camera_number: cameraNumber, framerate }));
        } else if (type === "rtmp") {
          const size = rtmpSizeRef.current[0] > 0 && rtmpSizeRef.current[1] > 0 ? rtmpSizeRef.current : undefined;
        } else if (type === "rtmp") {
          const size = rtmpSizeRef.current[0] > 0 && rtmpSizeRef.current[1] > 0 ? rtmpSizeRef.current : undefined;
          console.log("rtmp dispatch - rtmpSizeRef:", rtmpSizeRef.current, "size:", size);
          dispatch(updateElement({ name, id, type: "rtmp", location: [location[0] * props.sizeMultiplier, location[1] * props.sizeMultiplier], stream_name: streamName, framerate, size }));
        }
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
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

  async function handleLayerChange() {
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
    const element: Elem | undefined = configState.elements[configState.selectedElement - 1];
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
<<<<<<< HEAD
      }
=======
      } else if (element.type === "carousel") {
        setFilepaths(element.filepaths);
        setFramerate(element.framerate);
      } else if (element.type === "video") {
        setPath(element.filepath);
        setFramerate(element.framerate);
      } else if (element.type === "webcam") {
        setCameraNumber(element.camera_number);
        setFramerate(element.framerate);
      } else if (element.type === "rtmp") {
        setStreamName(element.stream_name);
        setFramerate(element.framerate);
        const s = element.size ?? [0, 0];
        setRtmpSize(s);
        rtmpSizeRef.current = s;      }
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
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
                onBlur={() => handleChange()}
                onKeyDown={(e) => {
                  if (e.key === "Enter") {
                    handleChange();
                  }
                }}

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
                onBlur={() => handleLayerChange()}
                onKeyDown={(e) => {
                  if (e.key === "Enter") {
                    handleLayerChange();
                  }
                }}

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
                  onBlur={() => handleChange()}
                  onKeyDown={(e) => {
                    if (e.key === "Enter") {
                      handleChange();
                    }
                  }}

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
                  onBlur={() => handleChange()}
                  onKeyDown={(e) => {
                    if (e.key === "Enter") {
                      handleChange();
                    }
                  }}

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
                onBlur={() => handleChange()}
                onKeyDown={(e) => {
                  if (e.key === "Enter") {
                    handleChange();
                  }
                }}

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
                onBlur={() => handleChange()}
                onKeyDown={(e) => {
                  if (e.key === "Enter") {
                    handleChange();
                  }
                }}

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
                onBlur={() => handleChange()}
                onKeyDown={(e) => {
                  if (e.key === "Enter") {
                    handleChange();
                  }
                }}

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
<<<<<<< HEAD
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
                onKeyDown={() => handleChange()}
                type="color"
                value={color}
              />
=======
          {type === "carousel" && (
            <li key={8} style={{ display: "flex" }}>
              <p className={styles.box} style={{ width: "25%" }}>filepaths</p>
              <textarea className={styles.box} style={{ width: "75%", backgroundColor: "whitesmoke", resize: "vertical", fontSize: "11px" }}
                value={filepaths.join("\n")}
                onChange={(e) => setFilepaths(e.target.value.split("\n").filter(Boolean))}
                onBlur={() => dispatch(updateElement({ name, id, type: "carousel", location: [location[0] * props.sizeMultiplier, location[1] * props.sizeMultiplier], filepaths, framerate }))}
                placeholder={"images/a.png\nimages/b.png"}
                rows={3} />
            </li>
          )}
          {type === "video" && (
            <li key={8} style={{ display: "flex" }}>
              <p className={styles.box} style={{ width: "25%" }}>filepath</p>
              <input className={styles.box} style={{ width: "75%", backgroundColor: "whitesmoke" }}
                onChange={(e) => setPath(e.target.value)}
                onKeyDown={(e) => handleChange(e)} type="text" value={path} />
            </li>
          )}
          {type === "webcam" && (
            <li key={8} style={{ display: "flex" }}>
              <p className={styles.box} style={{ width: "25%" }}>camera #</p>
              <input className={styles.box} style={{ width: "75%", backgroundColor: "whitesmoke" }}
                onChange={(e) => setCameraNumber(Math.max(0, e.target.valueAsNumber))}
                onKeyDown={(e) => handleChange(e)} type="number" value={cameraNumber} />
            </li>
          )}
          {type === "rtmp" && (
            <li key={8} style={{ display: "flex" }}>
              <p className={styles.box} style={{ width: "25%" }}>stream</p>
              <input className={styles.box} style={{ width: "75%", backgroundColor: "whitesmoke" }}
                onChange={(e) => setStreamName(e.target.value)}
                onKeyDown={(e) => handleChange(e)} type="text" value={streamName} />
            </li>
          )}
          {type === "rtmp" && (
            <li key={9} style={{ display: "flex" }}>
              <p className={styles.box} style={{ width: "24.5%" }}>size</p>
              <div className={styles.box} style={{ width: "75.5%", display: "flex", padding: "3px" }}>
                <p>w:</p>
                <input onChange={(e) => {   console.log("w onChange fired:", e.target.valueAsNumber);
const v: number[] = [e.target.valueAsNumber, rtmpSizeRef.current[1]]; setRtmpSize(v); rtmpSizeRef.current = v; }}                  onKeyDown={(e) => handleChange(e)} type="number" value={rtmpSize[0]}
                  style={{ width: "20%", backgroundColor: "whitesmoke", margin: "auto" }} />
                <p>h:</p>
                <input onChange={(e) => {   console.log("w onChange fired:", e.target.valueAsNumber);
const v: number[] = [rtmpSizeRef.current[0], e.target.valueAsNumber]; setRtmpSize(v); rtmpSizeRef.current = v; }}                  onKeyDown={(e) => handleChange(e)} type="number" value={rtmpSize[1]}
                  style={{ width: "20%", backgroundColor: "whitesmoke", margin: "auto" }} />
              </div>
            </li>
          )}
          {(type === "carousel" || type === "video" || type === "webcam" || type === "rtmp") && (
            <li key={10} style={{ display: "flex" }}>
              <p className={styles.box} style={{ width: "25%" }}>framerate</p>
              <input className={styles.box} style={{ width: "75%", backgroundColor: "whitesmoke" }}
                onChange={(e) => setFramerate(Math.max(1, e.target.valueAsNumber))}
                onKeyDown={(e) => handleChange(e)} type="number" value={framerate} />
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
            </li>
          )}
        </ul>
      </div>
    </div>
  );
}
export default DetailsPanel;
