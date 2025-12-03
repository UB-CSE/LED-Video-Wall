import { createSlice, type PayloadAction } from "@reduxjs/toolkit";

interface ConfigState {
    selectedElement: number;
    settings: Settings;
    elements: Elem[];
}
interface Settings {
    gamma: number;
}
interface Elem {
    name: string;
    id: number;
    type: string;
    filepath: string;
    location: number[];
    scale: number;
}
export type { Elem };

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
        resetState: (state) => {
            state.selectedElement = 0;
            state.elements = [];
            state.settings = {gamma: 0.0};
        },
    },
});
export const { setGamma, setSelectedElement, addElement, updateElement, resetState } = configSlice.actions;

export default configSlice.reducer;