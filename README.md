# YuriCable Pro Max

Implementation to use a Flipper-Zero as SWD/DCSD-Cable for iPhones just like known Kanzi/Kong/Tamarin Cables

> ![](docs/YuriCableLogo.svg)

## Docs

Here some specs about the protocol used:

* Name: SDQ (IDBUS) developed by Texas Instruments
* Source: [Reversed Protocol](https://nyansatan.github.io/lightning/)

Credits for SDQ reverse engineering to [@nyansatan](https://github.com/nyansatan)

## Project Setup

### Create .vscode

```shell
ufbt vscode_dist
```

### Open in CLion

Open the Project in CLion

## Flipper Docs

[Flipper Docs](https://docs.flipper.net/)

## Pinout Flipper Zero

![](docs/pinout.jpg)

## Pinout Lightning

![](docs/BreakoutPinout.jpg)
![](docs/Connector.jpg)

### Build

#### Build-Tool

```shell
python -m pip install --upgrade ufbt
```

#### Update Firmware

+ Download Update

```shell
ufbt update --channel=release
```

+ Upload to Flipper

```shell
ufbt flash_usb
```

#### Build

Navigiere in das Rootverzeichnis der App und führe dort nach installation von ufbt folgenden Befehl aus:

```shell
ufbt
```

Die `.fap`-Datei liegt dann im `./dist`-Ordner

#### Auto Launch

```shell
ufbt launch
```

#### UFBT

* Repo:  [UFBT GitHub](https://github.com/flipperdevices/flipperzero-ufbt)
* Docs:  [UFBT DOCS](https://github.com/flipperdevices/flipperzero-ufbt/blob/dev/README.md)

##### Raspberry Pi Pico Implementation

* [Tamarin Firmware](https://github.com/stacksmashing/tamarin-firmware)

py -m pip install --upgrade ufbt
```

#### Bauen der App

Navigiere in das Rootverzeichnis der App und führe dort nach installation von ufbt folgenden Befehl aus:

```txt
ufbt
```

Die `.fap`-Datei liegt dann im `./dist`-Ordner

#### UFBT

* Repo:  [UFBT GitHub](https://github.com/flipperdevices/flipperzero-ufbt)
* Docs:  [UFBT DOCS](https://github.com/flipperdevices/flipperzero-ufbt/blob/dev/README.md)

##### Raspberry Pi Pico Implementation 

* [Tamarin Firmware](https://github.com/stacksmashing/tamarin-firmware)
