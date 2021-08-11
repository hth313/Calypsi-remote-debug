Calypsi remote debug agent
==========================

This is a remote debug monitor or agent, implementing part of the gdbserver
protocol intended to be used with the Calypsi C compiler.

Currently there is support for the C256 Foenix U/U+ with a WDC65816.

Porting to other boards should be fairly straightforward by replacing
the low level communication functions.
