# Running ChatGPT on a Calculator
This program allows you to run ChatGPT on a calculator, specifically the TI 84 Plus CE. The program is written in C/C++ using the CE C/C++ Toolchain and requires a computer connected over USB via serial to function.
## Installation
To set up the program, first install the DEMO.8xp onto your calculator. This can be done by transferring the file to your calculator via a USB cable or by using a program like TI Connect CE. Once installed, launch the program by going to the catalog and selecting "asm(" and then opening the programs menu, selecting DEMO, and then pressing enter twice to run. The calculator should display "ready" when the program is ready to use.

Next, download the repository and run the Python program included. Select the COM port that your calculator is connected to. The calculator should display that it has connected.
## Usage
To enter a prompt, press the "2nd" key on your calculator. The prompt will be sent to the computer over serial, which will make the API call and pass the response back to the calculator over serial. The response will be displayed on the calculator screen.
