# Pico-Backscatter
An educational project on backscatter using Raspberry Pi Pico

## Repo Organization
- `hardware` contains the hardware design with further description and explanation.
- `baseband` contains a 2-FSK baseband code using Pico PIO together with a generator script, its description and some exercise questions.
- `project_pico_libs` contains the libs for all projects.
- `carrier-CC2500` contains a carrier generator using the Mikroe-1435 (CC2500) on the Pico
- `receiver-CC2500` contains a receiver using the Mikroe-1435 (CC2500) on the Pico
- `carrier-Firefly` contains the configuration guidance for home setup with zolertia firefly as carrier
- `carrier-characteristics` contains a measurement to estimate the typical carrier bandwidth.
- `carrier_receiver-CC1312` contains the configuration guidance for lab setup with CC1312 as carrier and/or receiver
- `carrier-receiver-baseband` integrates all components into one setup: the Pico generates the baseband, uses one Mikroe-1435 (CC2500) to generate a carrier and a second Mikroe-1435 (CC2500) to receive the backscattered signal.
- `stats` contains the system evaluation script

## Installation
A number of pre-requisites are needed to work with this repo:
Raspberry Pi Pico SDK, cmake and arm-none-eabi-gcc have to be installed for building and flashing the application code.
<br>Please follow the installation guidance in [Getting started with Rasberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).
<br>Using Visual Studio Code is recommended.


#### Comment
We recommend using the Raspberry Pi Pico SDK over Zephyr and Contiki for this project, since it integrates the required PIO toolchain. An example of using the PIO with Zephyr can be seen in the [pull request 44316](https://github.com/zephyrproject-rtos/zephyr/pull/44316/files), which is based on pre-assembling the state-machine with `pioasm` since the tool is not integrated to Zephyr.
