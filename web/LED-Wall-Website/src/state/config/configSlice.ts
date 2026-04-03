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
<<<<<<< HEAD
=======
    visible?: boolean; 
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
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
<<<<<<< HEAD
type Elem = ImageElem | TextElem;
export type { Elem };
export type { ImageElem };
export type { TextElem };
=======

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

export type { 
    Elem, ImageElem, TextElem, CarouselElem, 
    VideoElem, WebcamElem, RtmpElem 
};
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)

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
<<<<<<< HEAD
        clearElement: (state, action: PayloadAction<number>) => {
            state.elements = state.elements.filter((element) => element.id !== action.payload);
=======
        // NEW: Replaces the entire list. Crucial for duplication and dragging.
        reorderElements: (state, action: PayloadAction<Elem[]>) => {
            state.elements = action.payload;
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
        },
        updateElement: (state, action: PayloadAction<Elem>) => {
            const index = state.elements.findIndex(el => el.id === action.payload.id);
            if (index !== -1) {
                state.elements[index] = action.payload;
            }
        },
        updateLocation: (state, action: PayloadAction<{id: number, location: number[]}>) => {
            const element = state.elements.find(el => el.id === action.payload.id);
            if (element) {
                element.location = action.payload.location;
            }
        },
<<<<<<< HEAD
=======
        toggleElementVisibility: (state, action: PayloadAction<number>) => {
            const element = state.elements.find(el => el.id === action.payload);
            if (element) {
                element.visible = element.visible === false ? true : false;
            }
        },
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)
        resetState: (state) => {
            state.selectedElement = 0;
            state.elements = [];
            state.settings = { gamma: 0.0 };
        },
    },
});
<<<<<<< HEAD
export const { setGamma, setSelectedElement, addElement, clearElement, updateElement, resetState, updateLocation} = configSlice.actions;
=======

export const { 
    setGamma, 
    setSelectedElement, 
    addElement, 
    reorderElements, // Export this!
    updateElement, 
    resetState, 
    updateLocation, 
    toggleElementVisibility 
} = configSlice.actions;
>>>>>>> 4717901 (feat: implement add-to-top logic and all layer popups #169)

export default configSlice.reducer;