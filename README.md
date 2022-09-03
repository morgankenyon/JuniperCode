# JuniperCode
A project to hold juniper arduino code

## Getting Started

Lets get started with the basic `blink` example on Arduino.

* Install [Juniper Compiler](https://www.juniper-lang.org/index.html)
  * Ensure you have access to the `Juniper.exe` executable on your cli.
  * Will need to modify environmental variables to get that setup
* Install [PlatformIO](https://platformio.org/)
  * Ensure you have access to the `pio` executeable on your cli.
  * Will need to modify environmental variables to get that setup.
  * If not cloning this project, create platformio project with your specific board.
    * `pio project init --board uno` - for project targeting uno. (See platformio docs for more info)
* Load up the `blink.jun` file.
* In `/src/juniper`, run `compileAndUpload.ps1` script to compile to main.cpp in the compiled folder, then upload to your arduino.
  * The `compileAndUpload.ps1` contains the name of the juniper file to be compiled. So you'll have to change that whenever you want to compile something else.
  * You could use the `juniper/compileFile.ps1` and the `compiled/uploadToUno.ps1` folders separately to achieve the same thing, but why do extra work???
* You should see your Arduino blinking! Congrats on using Juniper!

## Juniper Help

While I am a programmer, I do not have a lot of experience writing in a functional programming language. And even less in Juniper. If you have any tips for how my programming can follow better funcational, reactive or Juniper paradigms, feel free to open a PR.