import { createSlice, type PayloadAction } from "@reduxjs/toolkit";

interface ConfigState {
    selectedElement: number;
    settings: Settings;
    elements: Elem[];
}
interface Settings {
    gamma: number;
}
interface BaseElem {
    name: string;
    id: number;
    location: number[];
    visible?: boolean;  // optional — undefined means visible (defaults to true)
}
interface ImageElem extends BaseElem {
    type: "image";
    filepath: string;
    scale: number;
}
interface TextElem extends BaseElem {
    type: "text";
    content: string;
    size: number;
    color: string;
    font_path: string;
}
interface CarouselElem extends BaseElem {
    type: "carousel";
    filepaths: string[];
    framerate: number;
}
interface VideoElem extends BaseElem {
    type: "video";
    filepath: string;
    framerate: number;
}
interface WebcamElem extends BaseElem {
    type: "webcam";
    camera_number: number;
    framerate: number;
}
interface RtmpElem extends BaseElem {
    type: "rtmp";
    stream_name: string;
    framerate: number;
    size?: number[];
}
type Elem = ImageElem | TextElem | CarouselElem | VideoElem | WebcamElem | RtmpElem;
export type { Elem };
export type { ImageElem };
export type { TextElem };
export type { CarouselElem };
export type { VideoElem };
export type { WebcamElem };
export type { RtmpElem };

const initialState: ConfigState = {
    selectedElement: 0,
    settings: {
        gamma: 0.0
    },
    elements: []
};

const configSlice = createSlice({
    name: "config",
    initialState,
    reducers: {
        setGamma: (state, action: PayloadAction<number>) => {
            state.settings.gamma = action.payload;
        },
        setSelectedElement: (state, action: PayloadAction<number>) => {
            state.selectedElement = action.payload;
        },
        addElement: (state, action: PayloadAction<Elem>) => {
            state.elements.push(action.payload);
        },
        reorderElements: (state, action: PayloadAction<Elem[]>) => {
            state.elements = action.payload;
        },
        updateElement: (state, action: PayloadAction<Elem>) => {
            for (let i = 0; i < state.elements.length; i++) {
                if (state.elements[i].id === action.payload.id) {
                    state.elements[i] = action.payload;
                }
            }
        },
        updateLocation: (state, action: PayloadAction<{id: number, location: number[]}>) => {
            for (let i = 0; i < state.elements.length; i++) {
                if (state.elements[i].id === action.payload.id) {
                    state.elements[i].location = action.payload.location;
                }
            }
        },
        toggleElementVisibility: (state, action: PayloadAction<number>) => {
            for (let i = 0; i < state.elements.length; i++) {
                if (state.elements[i].id === action.payload) {
                    // undefined means visible, so toggling undefined → false
                    state.elements[i].visible = state.elements[i].visible === false ? true : false;
                }
            }
        },
        resetState: (state) => {
            state.selectedElement = 0;
            state.elements = [];
            state.settings = {gamma: 0.0};
        },
    },
});
export const { setGamma, setSelectedElement, addElement, reorderElements, updateElement, resetState, updateLocation, toggleElementVisibility } = configSlice.actions;
export default configSlice.reducer;