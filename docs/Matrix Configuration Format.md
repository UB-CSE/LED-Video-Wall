# Matrix Configuration Format

The matrix configuration YAML file describes the physical placement of the LED matrices on the video wall. It also describes which microcontroller clients control which matrices.

See example matrix configuration YAML files in the `server/matrix-configs` directory.

The YAML is broken up into four parts:
- [`settings`](#settings)
- [`matrix-specs`](#matrix+specs)
- [`matrices`](#matrices)
- [`clients`](#clients)

## Settings
Labeled `settings` in the YAML file, this section must set the following properties:
- `ignore-bounds-checks`: Set `true` or `false`, determines whether the server will produce an error if canvas elements are placed outside of the canvas.
- `ns-per-frame`: The time period in between frames, in nanoseconds
- `ledvw-port`: The TCP port used to communicate with the microcontroller clients. To walls should not share the same port number.
- `rtmp-port`: The TCP port used for receiving RTMP streams.  To walls should not share the same port number.
- `brightness-percent`: 0 to 100 value for how bright the matrices should be.
- `image-encoding`: Format to encode images into before sending them to microcontroller clients. Available options are:
	- `rgb24`
	- `rgb21`
	- `rgb18`
	- `rgb15`
	- `rgb12`
	- `rgb9`
	- `rgb6`
	- `rgb3`
	- `yuv444`
	- `yuv422`
	- `yuv420`

## Matrix Specs
Labeled `matrix-specs` in the YAML file, this section defines which matrices are used on the video wall. For example, a wall that uses 64x64-pixel P3 matrices would define a matrix named `p3:64x64` like so (name does not matter):

```yaml
matrix-specs:
  "p3:64x64":
    power-limit-amps: 5.0
    dimensions:
      width: 64
      height: 64
```

## Matrices
Labeled `matrices` in the YAML file, this section lists the position and orientation of every LED matrix on the video wall.

Each matrix must be given the following:
- A unique ID
- `spec`: The name of the matrix spec defined in [`matrix-specs`](#matrix+specs)
- `pos`: The X, Y pixel location of the top-left corner of the matrix on the video wall
- `rot`: The rotation of the matrix: `up`, `down`, `left`, or `right`

Example:
```yaml
matrices:
  mat_p3_1:
    spec: "p3:64x64"
    pos: [0, 0]
    rot: down
  mat_p3_2:
    spec: "p3:64x64"
    pos: [0, 64]
    rot: down
```

## Clients
Labeled `clients` in the YAML file, this section specifies which matrices are controlled by each microcontroller client.

The MAC address of each client must be provided to identify it.

For WS2812B led matrices, specify the GPIO pin number connected to each matrix, like so:

```yaml
clients:
  AA-BB-CC-DD-EE-FF:
    matrix-connections:
      - pin: 16
        matrices: [mat1]
      - pin: 17
        matrices: [mat2]
```

For P3 Hub75 matrices, use negative pin numbers instead identify daisy-chained matrices, like so:

```yaml
clients:
  AA-BB-CC-DD-EE-FF:
    matrix-connections:
      - pin: -1
        matrices: [mat_p3_1]
      - pin: -2
        matrices: [mat_p3_2]
```
