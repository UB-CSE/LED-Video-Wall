import { type Dispatch } from "react";
import { addElement } from "../state/config/configSlice.ts";
import type { UnknownAction } from "@reduxjs/toolkit";
import type { RootState } from "../state/store.ts";

async function uploadFile(
  location: number[],
  file: File | null,
  dispatch: Dispatch<UnknownAction>,
  configState: RootState["config"]
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

      //Adds new element using the filename to the redux config
      dispatch(
        addElement({
          name: "elem" + String(configState.elements.length + 1),
          id: configState.elements.length + 1,
          type: "image",
          filepath: "images/" + filename,
          location: location,
          scale: 1,
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
