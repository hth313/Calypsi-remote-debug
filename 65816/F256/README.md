# F256 debug agent

This variant builds a debug agent for the F256 with a 65816 and flat memory load.
It has been tested on an F256Jr.

## Building

Type `make` in this directory, this creates a 8K image named
`agent-F256.raw`.

This is linked to use the address range `F000`-`FFFF` (in bank 0). The
`FF00`-`FFFF` is mirrored from the flash which contains the vectors
and some stub code. The actual debug agent code is located in the
upper 8K of the flash (sector `3F`), or `FF E000`-`FF FFFF`.

## Notes

As the interrupt vector is owned by the monitor and stored in flash
there is current no way to install your own interrupt handler.

RAM buffers and such use for communication to the host debugger is
located in `F000`-`FFEF`. In other words, your application code needs
to avoid the upper 4K of bank `00`.

## Installing

Once built you can download it using a command like:

```
./fm.sh --port /dev/ttyUSB0 --flash-sector 3f  --flash path-to/agent-F256.raw  --target=f256jr
```
