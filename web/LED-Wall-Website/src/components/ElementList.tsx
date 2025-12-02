import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useDispatch } from "react-redux";
import { setSelectedElement } from "../state/config/configSlice.ts";
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

function ElementList(props: Props) {
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

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

  const deleteOptions = [{ name: "delete", function: deleteElement }];
  const addOptions = [
    { name: "image", function: addImage },
    { name: "text", function: addText },
  ];

  const [contextOptions, setContextOptions] = useState<Option[]>(deleteOptions);

  function handleClick(id: number) {
    dispatch(setSelectedElement(id));
  }

  function handleRightClick(e: React.MouseEvent<HTMLLIElement, MouseEvent>) {
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
          {configState.elements.map((element) => (
            <li
              onClick={() => handleClick(element.id)}
              onContextMenu={(e) => handleRightClick(e)}
              key={element.id}
              style={{
                display: "flex",
                border:
                  configState.selectedElement == element.id
                    ? "3px solid cornflowerblue"
                    : "none",
              }}
            >
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
          ))}
        </ul>
      </div>
      <header style={{ display: "flex" }}>
        <h3>front</h3>
      </header>
      {contextIsClicked && (
        <ContextMenu
          options={contextOptions}
          location={contextLocation}
        ></ContextMenu>
      )}
      {addImageIsClicked && (
        <AddImagePopup
          sizeMultiplier={props.sizeMultiplier}
          setAddImageIsClicked={setAddImageIsClicked}
        ></AddImagePopup>
      )}
      {addTextIsClicked && (
        <AddTextPopup
          sizeMultiplier={props.sizeMultiplier}
          setAddTextIsClicked={setAddTextIsClicked}
        ></AddTextPopup>
      )}
    </div>
  );
}
export default ElementList;
