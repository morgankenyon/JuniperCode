# JuniperCode
A project to hold juniper arduino code

## Getting Started

* Install [Juniper Compiler](https://www.juniper-lang.org/index.html)
  * Ensure you have access to the `Juniper.exe` executable on your cli.
  * Will need to modify environmental variables to get that setup
* Install [PlatformIO](https://platformio.org/)
  * Ensure you have access to the `pio` executeable on your cli.
  * Will need to modify environmental variables to get that setup.
  * If not cloning this project, create platformio project with your specific board.
    * `pio project init --board uno` - for project targeting uno. (See platformio docs for more info)
* Load up the `blink.jun` file.
* In `/src/juniper`, run `compileFile.ps1` script to compile to main.cpp
* In `/src/compiled`, run `uploadToUno.ps1` script to upload to Arduino. 
* You should see your Arduino blinking! Congrats on using Juniper!
