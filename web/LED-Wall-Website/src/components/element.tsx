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
        setStartX(e.clientX);
        setStartY(e.clientY);
    }
    function handleDragEnd(e : React.DragEvent){
        setx(e.clientX - startX);
        sety(e.clientY - startY);
    }
    return (
        <div className="box" 
        style={{position: 'absolute', left: x, top: y}}>
            <img src={props.url} 
            draggable
            onDrag={(e) => handleDrag(e)}
            onDragStart={(e) => handleDragStart(e)}
            onDragEnd={(e) => handleDragEnd(e)}
            style={{width:props.size, height:props.size}}/>
        </div>
    );
}
export default Element;