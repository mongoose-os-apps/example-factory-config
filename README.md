# Example for device-based factory config overrides

This project illustrates how to work with device-based override.

It defines an RPC to set the factory confguration, e.g.:

```
$ mos call Test.SetFactoryConfig1 '{"foo": 234, "bar": "fff"}'
```

To add a part that zeroes out the config partition on initial flashing:
```
$ mos create-fw-bundle -i build/fw.zip -o build/fw_conf.zip conf:ptn=conf,fill=0xff,size=0x2000,encrypt=true
```

To embed a static config file conf1.json into firmware image (for debugging purposes, also not the explicit NUL termination of the file):
```
$ printf '\x00' >> conf1.json
$ mos create-fw-bundle -i build/fw.zip -o build/fw_conf.zip conf1:ptn=conf,src=conf1.json,encrypt=true
```
encrypt=true is needed if flash encryption is enabled and is ignored otherwise.

To examine raw device contents:

```
$ mos call Dev.Read '{"name": "conf", "offset": 0, "len": 128}' | jq -r .data | base64 -d | hexdump -C
00000000  7b 22 66 61 63 74 6f 72  79 22 3a 20 7b 22 66 6f  |{"factory": {"fo|
00000010  6f 22 3a 32 33 34 2c 22  62 61 72 22 3a 22 66 66  |o":234,"bar":"ff|
00000020  66 22 7d 7d 00 00 00 00  00 00 00 00 00 00 00 00  |f"}}............|
00000030  8a e9 53 7d b0 a1 40 ab  a7 50 8f ad ff ac 11 2c  |..S}..@..P.....,|
00000040  5d 68 16 a4 e8 2e ce 2a  54 86 26 26 e3 9b de cb  |]h.....*T.&&....|
...
```
