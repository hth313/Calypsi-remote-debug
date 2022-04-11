# Calypsi remote debug agent

This is a remote debug monitor or agent, implementing part of the gdbserver
protocol intended to be used with the Calypsi C compiler.

## Supported boards

The C256 Foenix U/U+ with a WDC65816 is supported using its serial
port at 115220.

The A2560 Foenix U/U+ with a 68000 is supported using its serial
port at 115220.

The HB68K08 single board computer with a 68008 is supported and its
serial port works at 57600.

## New targets

6502 support for the Commodore 64 using its VICE emulator has been
tried, but so far I have not managed to make the serial port work.
If you are interested, please give it a try getting it to work
with the Swiftlink serial port, either on VICE or a real Commodore 64.

Porting to other boards should be fairly straightforward by replacing
the low level communication functions.

The Foenix FMX should be fairly easy to add support for. It uses a
different serial port with different speed factors located at
another address.

The A2560K uses different addresses and a serial port with different
speed factors.



## Configure the port

On Linux you can use `dmesg` to figure out the serial port used on
your host computer:

```
   $ sudo dmesg | grep USB
   ...
   [109191.143623] ftdi_sio 1-1:1.0: FTDI USB Serial Device converter detected
   [109191.151619] usb 1-1: FTDI USB Serial Device converter now attached to ttyUSB1
```

Here you can see that a FTDI serial device is attached to
``/dev/ttyUSB1``.

You can configure the serial port on the host using:

```
$ stty 115200 -F /dev/ttyUSB1
```

An alternative is to use the `screen` utility program, which opens a
terminal session. This also sets up the serial port and you can also
test that the monitor responds to input.

```
$ screen /dev/ttyUSB1 115200
```

If you press a key the remote debug agent should respond with `$S13#b7`.

## Protocol snooping

When working with the debugger agent it can be useful to be able to
see the communications going on. You can use the
[interceptty program](https://github.com/geoffmeyers/interceptty)
for this.

Once installed you can start it using:

```
$ interceptty /devttyUSB1
```

This will allocate a pseudo tty and the number is dependent on your
system. One example of how to start debugger is to use:

```
$ db65816 --target-remote /dev/pts/8 application.elf
```

In this case you can inspect your /dev/pts directory to see what
devices you have before and after starting `interceptty`.
