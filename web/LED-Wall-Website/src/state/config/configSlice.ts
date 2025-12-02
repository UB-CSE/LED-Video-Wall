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
type Elem = ImageElem | TextElem;
export type { Elem };
export type { ImageElem };
export type { TextElem };

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
        resetState: (state) => {
            state.selectedElement = 0;
            state.elements = [];
            state.settings = {gamma: 0.0};
        },
    },
});
export const { setGamma, setSelectedElement, addElement, updateElement, resetState, updateLocation} = configSlice.actions;

export default configSlice.reducer;