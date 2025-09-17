import type React from "react";
import { useState } from "react";

type UrlProps = {
    url: string;
    size: number;
};
function Element(props : UrlProps){
    const [x, setx] = useState(0);
    const [y, sety] = useState(0);
    const [startX, setStartX] = useState(0);
    const [startY, setStartY] = useState(0);
    function handleDrag(e : React.DragEvent){
        setx(e.clientX - startX);
        sety(e.clientY - startY);
    }
    function handleDragStart(e : React.DragEvent){
        setStartX(e.clientX - x);
        setStartY(e.clientY - y);
    }
    function handleDragEnd(e : React.DragEvent){
        setx(e.clientX - startX);
        sety(e.clientY - startY);
        fetch('/api', {
            method: 'POST',
            body: JSON.stringify({
                x: e.clientX - startX,
                y: e.clientY - startY
            })})
    }
    return (
        <div className="box" 
        style={{position: 'fixed', left: x, top: y, width : `200px`, height : `200px`}}>
            <img src={props.url} 
            draggable
            onDrag={(e) => handleDrag(e)}
            onDragStart={(e) => handleDragStart(e)}
            onDragEnd={(e) => handleDragEnd(e)}
            style={{width: `${props.size}%`, height: `${props.size}%`}}/>
        </div>
    );
}
export default Element;