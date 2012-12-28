This is a rs485 device and protocol library. Atleast Maple is supported. Possibly AVR.
Written by Oskari Rauta.
License: expat ( check copying.txt )

In MasterTest folder is a pc program for testing. Install rs485 dongle to mac and compile
master. Install rs485 module to Serial1 and connect rts to pin9. Upload provided example
and start master on your pc/mac. Master has not been tested on pc, so there might be
compilation issues, I've only tested it on mac and I know it's a bit ugly, but I needed
something to get started and it does the job.

Ideology:
rs485 slaves get initially phone number 99 - they are started one by one with relays
and then server sets their phone numbers. Master does not handle any relay boards but
does exactly what I described for one device.

To download: click the DOWNLOADS button in the top right corner, rename the uncompressed 
folder RS485_library. Check that the folder contains RS485.cpp and RS485.h. Place the library 
folder your <maplesketchfolder>/libraries/ folder. You may need to create the libraries 
subfolder if its your first library. Restart the IDE. Connect rs485 module to serial1 and 
it's RTS to pin9.
