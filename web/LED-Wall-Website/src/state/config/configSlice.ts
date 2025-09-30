import { createSlice } from "@reduxjs/toolkit";

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
    reducers: {}
});

export default configSlice.reducer;