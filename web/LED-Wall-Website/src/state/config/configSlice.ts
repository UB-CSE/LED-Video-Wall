import { createSlice, type PayloadAction } from "@reduxjs/toolkit";

interface ConfigState {
    settings: Settings;
    elements: Element[];
}
interface Settings {
    gamma: number;
}
interface Element {
    name: string;
    id: number;
    type: string;
    filepath: string;
    location: number[];
}

const initialState: ConfigState = {
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
        addElement: (state, action: PayloadAction<Element>) => {
            state.elements.push(action.payload);
        },
        updateElement: (state, action: PayloadAction<Element>) => {
            for (let i = 0; i < state.elements.length; i++) {
                if (state.elements[i].id == action.payload.id) {
                    state.elements[i] = action.payload;
                }
            }
        },
    },
});
export const { setGamma, addElement, updateElement } = configSlice.actions;

export default configSlice.reducer;