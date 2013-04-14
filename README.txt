Turns the Arduino into an interface (I/O) for Virtual Pinball Cabinets in hope to 
replace the LedWiz for controling the differents "toys" (ouputs) and the leds. 
As for inputs, it will/should replace a keyboard controller like the Ipac, a 
nice save of 40$! :D
		 
It should also be much more fast/powerfull than the LedWiz, thus preventing the
lag/stutter that can happen on some tables. Not bad since they're both about the 
same price (depending on the Arduino model)!
		 
This Arduino program sends data thru the serial connection (no custom USB HID
or things like that), so it as to work in pair with a "listening" program on the
PC that will translate the I/O commands sent thru that serial port.
		 
Basically, the Arduino should/will only read and write to it's pin without doing
much more than that. The core of the program will be the PC interface interpreting
the commands (sending keypresses and telling arduino which toys to activate) 
by interfacing with the pinball softwares (Visual pinball, Future Pinball, and I 
guess then Hyperpin frontend too).

I don't know why I'm saying all that since I'm basically writing to myself.
		 
The PC program should include a configuration file/interface to make the translation
from pins to I/O more easy and preventing bad "hardcoded" coding. As a career software
developper, I have that user friendly mentality always on the back of my head. So it
should not be too hard to configure. 
		 
But I also do this for free.. so keep in mind that this program doesn't come with a 
waranty or client care. I'm a nice guy, but I also like doing nothing when I can.	