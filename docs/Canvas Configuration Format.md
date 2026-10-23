# Canvas Configuration Format

The canvas configuration YAML file describes the canvas elements to be shown on the video wall.

See example canvas configuration YAML files in the `server/canvas-configs` directory.

The YAML is broken up into two parts:
- [`settings`](#settings)
- [elements](#elements)
## Settings

There is currently only one property, `gamma`. 1.0 is default. Less than 1.0 makes the image lighter, and greater than 1.0 makes the image darker.

## Elements

A list of elements on the canvas.

Each element has the following common properties available:
- The unique ID of the element
- `order`: The order of the element (0 is bottom)
- `name`: A user-friendly display name for the element
- `location`: Pixel location of the element
- `rotation`: Rotation of the element, in degrees. Positive is counter-clockwise.
- `size`: The pixel size of the element
- `preserve-aspect-ratio`: When `true`, the element will try to preserve its original aspect ratio and not stretch when `size` is set to a different aspect ratio. Instead, `size` will instead as a "bounding box" for the element.

The `type` property specifies the element's type:
- [`image`](#image)
- [`carousel`](#carousel)
- [`video`](#video)
- [`rtmp`](#rtmp)
- [`web-browser`](#web+browser)
- [`text`](#text)

#### Image
- `filepath`: The image file path

#### Carousel
- `filepaths`: A list of image file paths
- `framerate`: Rate in Hz to cycle through the images

#### Video
- `filepath`: The video file path
- `framerate`: Video framerate in Hz

#### RTMP
- `stream-name`: The name of the RTMP stream to display
- `framerate`: The stream framerate in Hz

#### Web Browser
- `url`: The URL to display
- `framerate`: The framerate of the element, in Hz
- `view-size`: The size to render the webpage at (`size` is still the display size on the video wall)
- `cookies`: A list of cookies to set. Each cookie has the following properties:
	- `name`: Name
	- `value`: Value
	- `domain`: Domain
	- `path`: Path
	- `secure`: Secure, true of false
	- `httpOnly`: HttpOnly, true of false
	- `sameSite`: SameSite, either `None`, `Lax`, or `Strict`

#### Text
- `text`: Content to display
- `font-path`: Path to font file
- `font-size`: Font size
- `color`: Hex color
