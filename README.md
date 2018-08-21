# toxirc
Tox to IRC bridge with support for multiple channels

## Features

* Mutliple IRC channels
* Commands
* Custom prefixes
* Registered IRC accounts
* Human readable settings file (INI)

## Dependencies

* Toxcore
* pkg-config (for building)

## Build

```sh
git clone --recursive https://github.com/endoffile78/toxirc
cd toxirc
make
```

## Usage

 After building toxirc copy example\_settings.ini file to settings.ini:
 ```sh
 cp settings_example.ini settings.ini
 ```

 Afterwards modify the settings file, then run toxirc. If you would like the default settings just run toxirc.

## Tox ID

`A922A51E1C91205B9F7992E2273107D47C72E8AE909C61C28A77A4A2A115431B14592AB38A3B`

## License

[MIT](LICENSE)
