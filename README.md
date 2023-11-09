# Flipper Zero DCSD Cable

Implementation to use a Flipper-Zero as DCSD-Cable for iPhones

## Docs

Here some specs about the protocol used:

* Name: SDQ (IDBUS) developed by Texas Instruments
* Source: [Reversed Protocol](https://nyansatan.github.io/lightning/)

## Flipper Docs

[Flipper Docs](https://docs.flipper.net/)

## Pinout Flipper Zero

![](assets/EnLvCM5-J45B9sBm6UBig_flipper-zero-gpio-pinout-flipper-zero-12f7b9c6-drawing-rev-04.jpg)

### Build

#### Installation des Build-Tools

Linux & macOS:

```sh
python3 -m pip install --upgrade ufbt
```

Windows:

```sh
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
