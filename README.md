# pi-gyromouse

MP6050 gyro-controlled mouse "driver" and associated tools for the Raspberry Pi, as written about here: http://www.allaboutcircuits.com/technical-articles/gyromouse-creating-novel-linux-controllers/

so long as you have all the usual linux C compiler tools installed, you should just be able to 'make' in each of the directories, and then look at "gyrmouse/run.sh" for some examples of how to chain the tools together.

The programs are split so that lower level "device drivers" talk to the hardware you have, and the "mouse" programs get the abstracted data stream from the device (by just piping the output of one to the others) to finally send pointer messages to X-windows. There are excellent comp.sci reasons for this split that I won't get into here.

This isn't supposed to be compile/install/go... you're unlikely to have exactly the same hardware and requirements I did. This is supposed to be a example for you to create your own novel device drivers.

Some of the directories even have early _javascript_ versions of the device drivers that run under node.js, but I rewrote them later in C for efficiency reasons. (Like, 30% CPU down to 3%.) And no depedancy on node.js for your mouse to boot. But they're there if you want a laugh.

The gyromouse program also has a kind of "screensaver" feature that calls the "screen_on.sh" and "screen_off.sh" scripts when the mouse is moved, or goes idle. This is partly because X-windows was being a pain reporting real screensaver events, I needed to aggresively turn off the screen for power reasons which meant scripting hardware port-flips anyway, and I ran out of time.

