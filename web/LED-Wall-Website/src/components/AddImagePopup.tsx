import styles from "../Styles.module.css";
import { useState, type ChangeEvent } from "react";
import uploadFile from "./Upload.tsx";
import { useDispatch } from "react-redux";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";

type Props = {
  sizeMultiplier: number;
  setAddImageIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddImagePopup(props: Props) {
  const [preview, setPreview] = useState(false);
  const [imageUrl, setImageUrl] = useState("");
  const [newFile, setFile] = useState<File | null>(null);
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

  //Sets the file
  async function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    fileChange(e.dataTransfer.files[0]);
  }

  //Prevents browser not allowing dragging the image
  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
  }

  async function fileChange(file: File) {
    if (!file || file.size === 0) {
      return;
    }
    setFile(file);
    const reader = new FileReader();
    reader.readAsDataURL(file);
    reader.onload = () => {
      setPreview(true);
      if (typeof reader.result === "string") {
        setImageUrl(reader.result);
      }
    };
  }

  function handleClose() {
    setPreview(false);
    setImageUrl("");
    setFile(null);
  }

  function handleChange(e: ChangeEvent<HTMLInputElement>) {
    if (e.target.files) {
      fileChange(e.target.files[0]);
    }
  }

  function handleUpload() {
    if (newFile) {
      uploadFile([0, 0], newFile, dispatch, configState);
      props.setAddImageIsClicked(false);
    }
  }

  const uploadContent = (
    <label
      className={styles.download}
      htmlFor="img-upload"
      onDrop={(e) => {
        handleDrop(e);
      }}
      onDragOver={(e) => {
        handleDragOver(e);
      }}
    >
      <svg
        id="Layer_1"
        data-name="Layer 1"
        xmlns="http://www.w3.org/2000/svg"
        viewBox="0 0 122.88 120.89"
        width="100px"
        height="100px"
        style={{ fill: "rgb(29, 41, 58)" }}
      >
        <title>download-file</title>
        <path d="M84.58,47a7.71,7.71,0,1,1,10.8,11L66.09,86.88a7.72,7.72,0,0,1-10.82,0L26.4,58.37a7.71,7.71,0,1,1,10.81-11L53.1,63.12l.16-55.47a7.72,7.72,0,0,1,15.43.13l-.15,55L84.58,47ZM0,113.48.1,83.3a7.72,7.72,0,1,1,15.43.14l-.07,22q46,.09,91.91,0l.07-22.12a7.72,7.72,0,1,1,15.44.14l-.1,30h-.09a7.71,7.71,0,0,1-7.64,7.36q-53.73.1-107.38,0A7.7,7.7,0,0,1,0,113.48Z" />
      </svg>
      <p style={{ margin: "0px" }}>Upload image no larger than 5mb</p>
      <input
        type="file"
        name="Add Element"
        accept="image/*"
        id="img-upload"
        onChange={handleChange}
        style={{ display: "none" }}
      />
    </label>
  );
  const previewImage = (
    <div className={styles.preview}>
      <img src={imageUrl} alt="" />
      <div className={styles.overlay}>
        <button className={styles.close} onClick={handleClose}>
          <svg
            version="1.1"
            id="Layer_1"
            xmlns="http://www.w3.org/2000/svg"
            xmlnsXlink="http://www.w3.org/1999/xlink"
            x="0px"
            y="0px"
            width="32px"
            height="32px"
            viewBox="0 0 122.881 122.88"
            enable-background="new 0 0 122.881 122.88"
            xmlSpace="preserve"
          >
            <g>
              <path
                fill="whitesmoke"
                d="M61.44,0c16.966,0,32.326,6.877,43.445,17.996c11.119,11.118,17.996,26.479,17.996,43.444 c0,16.967-6.877,32.326-17.996,43.444C93.766,116.003,78.406,122.88,61.44,122.88c-16.966,0-32.326-6.877-43.444-17.996 C6.877,93.766,0,78.406,0,61.439c0-16.965,6.877-32.326,17.996-43.444C29.114,6.877,44.474,0,61.44,0L61.44,0z M80.16,37.369 c1.301-1.302,3.412-1.302,4.713,0c1.301,1.301,1.301,3.411,0,4.713L65.512,61.444l19.361,19.362c1.301,1.301,1.301,3.411,0,4.713 c-1.301,1.301-3.412,1.301-4.713,0L60.798,66.157L41.436,85.52c-1.301,1.301-3.412,1.301-4.713,0c-1.301-1.302-1.301-3.412,0-4.713 l19.363-19.362L36.723,42.082c-1.301-1.302-1.301-3.412,0-4.713c1.301-1.302,3.412-1.302,4.713,0l19.363,19.362L80.16,37.369 L80.16,37.369z M100.172,22.708C90.26,12.796,76.566,6.666,61.44,6.666c-15.126,0-28.819,6.13-38.731,16.042 C12.797,32.62,6.666,46.314,6.666,61.439c0,15.126,6.131,28.82,16.042,38.732c9.912,9.911,23.605,16.042,38.731,16.042 c15.126,0,28.82-6.131,38.732-16.042c9.912-9.912,16.043-23.606,16.043-38.732C116.215,46.314,110.084,32.62,100.172,22.708 L100.172,22.708z"
              />
            </g>
          </svg>
        </button>
      </div>
    </div>
  );
  return (
    <div
      className={styles.popup}
      onClick={(e) => {
        e.stopPropagation();
      }}
    >
      {preview ? previewImage : uploadContent}
      <button
        onClick={handleUpload}
        style={{ left: "0%", transform: "translate(0%, 0%)" }}
      >
        Add
      </button>
    </div>
  );
}

export default AddImagePopup;
