# Video Wall Server Command Reference

## Running Commands

There are two ways to invoke LEDVW server commands:
1. Send commands to the unix socket at `/tmp/ledvw-cmd-<wall-name>`
	- Use web server or `led-video-wall-cmd-tester` program for testing
2. When running with `--interactive`, entering commands into the GNU readline prompt

Commands are invoked the same as you would in bash, with the format
```
command-name arg1 "arg 2" ...
```
Arguments with spaces can be surrounded with quotes.

## Commands
### quit/exit
Perform a clean server shutdown.
### pause
Pause the virtual canvas and keep displaying only the current frame.
### resume
Resume the virtual canvas and continue rendering new frames.
### canvas-clear
Remove all elements from the canvas.
### canvas-load \<filepath\>
##### Arguments:
- __filepath__: Path to the canvas configuration YAML file to load.

Clear the virtual canvas and load elements from the specified configuration.
### canvas-save \<filepath\>
##### Arguments:
- __filepath__: Path to the canvas configuration YAML file to save.

Save the current canvas state to the specified configuration file.
### canvas-get-elements
Responds with a JSON containing all the current canvas elements.
### element-delete \<id\>
##### Arguments:
1. __id__: The ID of the element to delete

Deletes an element from the virtual canvas.
### element-rename \<id\> \<name\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __name__: The new name of the element

Renames an element.
### element-move \<id\> \<x\> \<y\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __x__: The new X pixel coordinate of the element
3. __y__: The new Y pixel coordinate of the element

Moves an element.
### element-move-up \<id\>
##### Arguments:
1. __id__: The ID of the element to move

Brings an element up one level in the layer stack.
### element-move-down \<id\>
##### Arguments:
1. __id__: The ID of the element to move

Drops an element down one level in the layer stack.
### element-resize \<id\> \<width\> \<height\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __width__: The new pixel width of the element
3. __height__: The new pixel height of the element

Resizes an element. If the `preserve aspect ratio` property is set to true, this will only act as a "bounding box" for the element and will not stretch it.
### element-rotate \<id\> \<degrees\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __degrees__: The new rotation of the element, in degrees (positive => counter-clockwise)

Rotates an element.
### element-set-preserve-aspect-ratio \<id\> \<true|false\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __true__ or __false__: Whether the element's original aspect ratio should be preserved when resizing.

Sets whether the element's original aspect ratio should be preserved when resizing.
### image-new \<filepath\>
##### Arguments:
1. __filepath__: The image file path

Create a new image element with the given image.
### image-set-file \<id\> \<filepath\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __filepath__: The image file path

Assigns the image file path of an image element.
### carousel-new \<framerate\> \[filepaths...\]
##### Arguments:
1. __framerate__: Rate in Hz to cycle through the images
2. __\[filepaths...\]__: A list of image file paths

Create a new carousel element with the given properties.
### carousel-set-files \<id\> \[filepaths...\]
##### Arguments:
1. __id__: The ID of the element to edit
2. __\[filepaths...\]__: A list of image file paths

Assigns the image file paths of a carousel element.
### carousel-set-framerate \<id\> \<framerate\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __framerate__: Rate in Hz to cycle through the images

Assigns the framerate of a carousel element.
### rtmp-new \<stream-name\> \<framerate\>
##### Arguments:
1. __stream-name__: The name of the RTMP stream (ex. "test" when streaming to rtmp://yoshi.cse.buffalo.edu/test)
2. __framerate__: The stream framerate in Hz

Create a new RTMP Stream element with the given properties.
### rtmp-set-framerate \<id\> \<framerate\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __framerate__: The stream framerate in Hz

Assigns the framerate of a RTMP stream element.
### rtmp-set-stream-name \<id\> \<stream-name\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __stream-name__: The name of the RTMP stream (ex. "test" when streaming to rtmp://yoshi.cse.buffalo.edu/test)

Assigns the stream name of a RTMP stream element.
### text-new \<content\> \<font-path\> \<font-size\> \<r\> \<g\> \<b\>
##### Arguments:
1. __content__: Text content to be displayed
2. __font-path__: Font file path
3. __font-size__: Font size
4. __r__: Red color channel value (0-255)
5. __g__: Green color channel value (0-255)
6. __b__: Blue color channel value (0-255)

Create a new text element with the given properties.
### text-set-content \<id\> \<content\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __content__: Text content to be displayed

Assigns the content of a text element.
### text-set-font \<id\> \<filepath\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __filepath__: Font file path

Assigns the font of a text element.
### text-set-font-size \<id\> \<font-size\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __font-size__: Font size

Assigns the font size of a text element.
### text-set-color \<id\> \<r\> \<g\> \<b\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __r__: Red color channel value (0-255)
3. __g__: Green color channel value (0-255)
4. __b__: Blue color channel value (0-255)

Assigns the color of a text element.
### video-new \<filepath\> \<framerate\>
##### Arguments:
1. __filepath__: The video file path
2. __framerate__: Video framerate in Hz

Create a new video element with the given properties.
### video-set-file \<id\> \<filepath\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __filepath__: The video file path

Assigns the video file path of a video element.
### video-set-framerate \<id\> \<framerate\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __framerate__: Video framerate in Hz

Assigns the framerate of a video element.
### web-browser-new \<url\> \<framerate\> \<view-width\> \<view-height\>
##### Arguments:
1. __url__: The URL to display
2. __framerate__: Web browser framerate in Hz
3. __view-width__: Width to render the webpage at
4. __view-height__: Height to render the webpage at

Create a new web browser element with the given properties.
### web-browser-set-url \<id\> \<url\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __url__: Video framerate in Hz

Assigns the URL of a web browser element.
### web-browser-set-framerate \<id\> \<framerate\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __framerate__: Video framerate in Hz

Assigns the framerate of a web browser element.
### web-browser-set-view-size \<id\> \<width\> \<height\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __view-width__: Width to render the webpage at
3. __view-height__: Height to render the webpage at

Assigns the view size of a web browser element (note that `size` is still the display size on the video wall).
### web-browser-set-cookie \<id\> \<name\> \<value\> \<domain\> \<path\> \<secure\> \<httpOnly\> \[sameSite\]
##### Arguments:
1. __id__: The ID of the element to edit
2. __Name__: Cookie name
3. __Value__: Cookie value
4. __Domain__: Cookie domain
5. __Path__: Cookie path
6. __Secure__: Cookie secure, true or false
7. __HttpOnly__: Cookie HttpOnly, true or false
8. __SameSite__: Cookie SameSite,  either `None`, `Lax`, or `Strict`

Sets a cookie for a web browser element.
### web-browser-delete-cookie \<id\> \<cookie-name\>
##### Arguments:
1. __id__: The ID of the element to edit
2. __cookie-name__: Name of the cookie to delete

Deletes a cookie from a web browser element.