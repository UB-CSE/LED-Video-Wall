import { type Dispatch } from "react";
import { reorderElements } from "../state/config/configSlice.ts";
import type { UnknownAction } from "@reduxjs/toolkit";
import type { RootState } from "../state/store.ts";

async function uploadFile(
  location: number[],
  file: File | null,
  dispatch: Dispatch<UnknownAction>,
  configState: RootState["config"]
) {
  if (file != null && file.size > 5000000) {
    return "File must be under 5MB";
  } else if (file != null) {
    const formData = new FormData();
    formData.append("file", file);
    try {
      const res = await fetch("/api/upload-file", {
        method: "POST",
        body: formData,
      });
      const json = await res.json();
      const filename = json["filename"];

      const newElement = {
        name: "elem" + String(configState.elements.length + 1),
        id: 1,
        type: "image" as const,
        filepath: "images/" + filename,
        location: location,
        scale: 1,
      };
      const shifted = configState.elements.map((el) => ({ ...el, id: el.id + 1 }));
      dispatch(reorderElements([newElement, ...shifted]));

      return "File uploaded successfully";
    } catch (err) {
      return "[Error]: invalid file";
    }
  }
}

export default uploadFile;