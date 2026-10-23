export default function Header() {
    return (
        <div className="header-controls">
            <span className="header-title">LED Video Wall</span>
            <span className="header-separator" aria-hidden="true"/>
            <button type="button">Demo Wall</button>
            <span className="status">CONNECTED</span>
            <span className="header-separator" aria-hidden="true"/>
            <button type="button">input.yaml</button>

            <label className="autosave">
                <input type="checkbox"/>
                Auto-Save
            </label>

            <button type="button">Save</button>
            <span className="status">UNSAVED</span>
        </div>
    )

}
