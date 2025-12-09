#DESCRIPTION

Curie is a cute desk companion robot. Its physical body consists of 3 servos to give it 3 dof, 2 touch capacitive touch sensors which allow you to pet it, and an oled screen to display its cute eyes.
My goal is to be able to speak to Curie through AI, preferably trough an api key and not locally as i want it to have access to the internet.
I want her to have "mood settings" so that i can choose Curie's mood to match mine. Sometimes you just need a study buddy, so all you gotta say is "Curie, study mode." She will then switch to said mode and unlock a set of features, like pomodoro in this case. Another funny mode could be the "Talking Curie mode" where it behaves much like Talking Ben, replying only with yes/no/laugh/etc randomly.These are only a few of the moods that i have in mind that i would like to implement.
Apart from this i want Curie to be highly customizable, perhaps through an app. For example, i want Curie to be able to reference memes that are popular and trending at this time freely based on the "Humor Setting" set by the user. 
This is a long term project and i will keep adding features to her as i get new ideas.

<img width="635" height="751" alt="image" src="https://github.com/user-attachments/assets/7f9b1ed6-d8ab-4d0d-b927-685365b9b905" />



#WIRING

###OLED Display (I2C)
VCC -> 3.3V Pin on ESP32
GND -> GND Pin on ESP32
SDA -> GPIO 21 on ESP32
SCL -> GPIO 22 on ESP32

###Servo 1 (X-Axis, Pan)
VCC (Red wire) -> External 5V Power (+)
GND (Brown/Black wire) -> GND Pin on ESP32
Data (Orange/Yellow wire) -> GPIO 12 on ESP32

###Servo 2 (Y-Axis, Tilt)
VCC (Red wire) -> External 5V Power (+)
GND (Brown/Black wire) -> GND Pin on ESP32
Data (Orange/Yellow wire) -> GPIO 13 on ESP32

###TTP223 Touch Sensor
VCC -> 3.3V Pin on ESP32
GND -> GND Pin on ESP32
I/O (or SIG) -> GPIO 18 on ESP32

###External 5V Power
7.4V li-po
use buck converter to step downt to 5V

#BODY

modified a servo bracket meant for 2-axis fpv control. Working on a new design that can incorporate 3 dof which will be 3d printed.
https://amzn.in/d/41nkgiY
