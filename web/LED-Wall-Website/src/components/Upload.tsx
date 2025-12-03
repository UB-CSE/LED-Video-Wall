import React, { type Dispatch, type JSX } from "react";
import { addElement } from "../state/config/configSlice.ts";
import Element from "./element";
import type { UnknownAction } from "@reduxjs/toolkit";

async function uploadFile(
  location: number[],
  file: File | null,
  sizeMultiplier: number,
  elements: JSX.Element[],
  setElements: React.Dispatch<React.SetStateAction<JSX.Element[]>>,
  dispatch: Dispatch<UnknownAction>
) {
  //We require that the image file must be under 5MB
  if (file != null && file.size > 5000000) {
    //setMessage("File must be under 5MB");
    return "File must be under 5MB";
  } else if (file != null) {
    const formData = new FormData();
    formData.append("file", file);
    try {
      //Sends file to the server and receives a new filename from the server
      const res = await fetch("/api/upload-file", {
        method: "POST",
        body: formData,
      });
      const json = await res.json();
      const filename = json["filename"];
      //Adds new element using the filename to the rendered Elements
      const newElement = (
        <Element
          key={elements.length + 1}
          name={"elem" + String(elements.length + 1)}
          id={elements.length + 1}
          type={file.type.split("/")[0]}
          path={"images/" + filename}
          location={[
            location[0] * sizeMultiplier,
            location[1] * sizeMultiplier,
          ]}
          sizeMultiplier={sizeMultiplier}
        />
      );
      setElements([...elements, newElement]);
      //Adds new element using the filename to the redux config
      dispatch(
        addElement({
          name: "elem" + String(elements.length + 1),
          id: elements.length + 1,
          type: file.type.split("/")[0],
          filepath: "images/" + filename,
          location: location,
        })
      );
      //setMessage("Success!");
      return "File uploaded successfully";
    } catch (err) {
      //setMessage("Server Error");
      return "[Error]: invalid file";
    }
  }
}

export default uploadFile;
