import type React from "react";
import { useState, useId} from "react";


type UrlProps = {
    url: string;
    size: number;
};
function Element(props : UrlProps){
    const [x, setx] = useState(0);
    const [y, sety] = useState(0);
    const [startX, setStartX] = useState(0);
    const [startY, setStartY] = useState(0);
    const element_id = useId();
    const [count, setCount] = useState(0);

    function handleDrag(e : React.DragEvent){
        setx(e.clientX - startX);
        sety(e.clientY - startY);
        if(count >= 50){
            sendPosition();
            setCount(0);
        } else {
            setCount(count + 1);
        }
    }
    function handleDragStart(e : React.DragEvent){
        e.dataTransfer.setDragImage(e.currentTarget, -1000, -1000);
        setStartX(e.clientX - x);
        setStartY(e.clientY - y);
    }
    function handleDragEnd(e : React.DragEvent){
        var newX = e.clientX - startX;
        var newY = e.clientY - startY;
        setx(newX);
        sety(newY);
        sendPosition();
    }
    function sendPosition(){
        fetch('/get_Data', {
            method: 'POST',
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({
                "id": element_id,
                "x": x,
                "y": y
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
            style={{width: `${props.size}%`, height: `${props.size}%`}}
            />
        </div>
    );
}
export default Element;