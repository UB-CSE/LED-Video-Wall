import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useDispatch } from "react-redux";
import { setSelectedElement } from "../state/config/configSlice.ts";
import type React from "react";
import { useState, type JSX } from "react";
import ContextMenu from "./ContextMenu.tsx";
import useContextMenu from "../hooks/useContextMenu.tsx";
import { type Option } from "./ContextMenu.tsx";
import AddImagePopup from "./AddImagePopup.tsx";

type Props = {
  elements: JSX.Element[];
  setElements: React.Dispatch<React.SetStateAction<JSX.Element[]>>;
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

  const deleteOptions = [{ name: "delete", function: deleteElement }];
  const addOptions = [{ name: "image", function: addImage }];

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

  return (
    <div className={styles.panel} style={{ height: "800px" }}>
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
      <ul style={{ paddingLeft: "0px", paddingRight: "60px" }}>
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
      {contextIsClicked && (
        <ContextMenu
          options={contextOptions}
          location={contextLocation}
        ></ContextMenu>
      )}
      {addImageIsClicked && (
        <AddImagePopup
          elements={props.elements}
          setElements={props.setElements}
          sizeMultiplier={props.sizeMultiplier}
          setAddImageIsClicked={setAddImageIsClicked}
        ></AddImagePopup>
      )}
    </div>
  );
}
export default ElementList;
