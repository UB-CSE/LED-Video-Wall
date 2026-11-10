# Video Wall Hardware Configuration

## P3 64x64 Matrix Video Wall

Hardware:

- Microcontroller: [Adafruit Matrix Portal S3 (WiFi)](https://www.adafruit.com/product/5778)
- LED Matrix: [64x64 P3 LED Matrix](https://www.adafruit.com/product/5407)
- Blade fuse box

Each microcontroller can control up to six daisy-chained 64x64 matrices (due to a memory limitation).

The microcontroller has a 2x10 socket connector to connect the first matrix:

<img src="images/p3-matrix-microcontroller.jpeg" width=400/>

The matrix needs its own power supply from the fuse box

<img src="images/p3-matrix-back.jpeg" width=400/>

Subsequent microcontrollers can be "daisy-chained" by connecting them to previous matrices, as seen above.

## WS2812B Matrix Video Wall

Hardware:

- Microcontroller: __ESP32__
- LED Matrix: __32x8 WS2812B Matrix__
- Blade fuse box

Each microcontroller controls a 32x32 square chunk of LEDs, consisting of four 32x8 WS2812B matrices.

The wiring of each chunk looks like this:

<img src="images/ws2812b-chunk-back.jpeg" width=400/>

On the bottom left is the fuse box where all power is routed. Each LED matrix is given a 5 Amp fuse.

The ESP32 microcontroller is on the bottom right. The signal pin of each LED matrix gets connected to a GPIO pin on the
microcontroller. Any GPIO pins will work but 16, 17, 18, and 19 are typically used. These pin numbers need to be
declared in the matrix configuration YAML file for the microcontroller to know what to control.

## Power Cables

The power cables that come with the P3 LED matrices may not be long enough to reach the fuse box. They can be extended
by following this procedure.

Cut the fork ends off the included power cable.

<img src="images/p3-cut-wire.png" width=400/>

Strip all four wires by about 2cm.

<img src="images/p3-strip-wire.png" width=400/>

Twist the pairs to improve rigidity.

<img src="images/p3-twist-wires.png" width=400/>

Cut an appropriate length of additional wire to reach the P3 matrix from the fuse box and split the ends. We're using 12
AWG wire here, which is overkill for this application.

<img src="images/p3-additional-wire.png" width=400/>

Strip one end by about 2cm and one end a little shorter. Twist all the stripped ends.

<img src="images/p3-additional-wire-stripped.png" width=400/>

Place a 16~14 AWG (blue) M4 fork connector over the shorter stripped ends. Ensure the copper wire reaches all the way
through it, and crimp it. Try to lightly pull the wire out to ensure it's tight.

<img src="images/p3-crimp-fork.png" width=400/>

Place a 10~12 AWG butt splice (yellow) connector between the additional wire and the provided wires. Feed the wires
through, and ensure they meet and overlap in the middle. Crimp the connector down, and ensure neither end can slide out.

<img src="images/p3-butt-connector.png" width=400/>

The cable is complete! You can now attach one end to the fuse box and the other to a P3 matrix.

<img src="images/p3-power-cable-complete.png" width=400/>
