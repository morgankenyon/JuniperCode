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
* Load up the `blink.jun` file in Visual Studio
* In `/src/juniper`, double check that `compileAndUpload.ps1` contains `blink.jun` as the compiled script name (line 5).
  * The `compileAndUpload.ps1` contains the name of the juniper file to be compiled. So you'll have to change that whenever you want to compile something else.
* Run `compileAndUpload` to compile the target .jun file to the `src/compiled/main.cpp` file, then your program will be uploaded to your arduino.
  * You could use the `juniper/compileFile.ps1` and the `compiled/uploadToUno.ps1` scripts separately to achieve the same thing, but why do extra work???
* You should see your Arduino blinking! Congrats on using Juniper!

## Further exploration

The juniper [README](./src/juniper/README.md) file contains information about further Juniper files you can play around with.

You can look there for inspiration, or if you have the electronics to build the circuits as best you can to control your arduino functionally.

## Juniper Help

While I am a programmer, I do not have a lot of experience writing in a functional programming language. And even less in Juniper. If you have any tips for how my programming can follow better funcational, reactive or Juniper paradigms, feel free to open a PR.