# haywire_checkm8

Little checkm8 rewrite in C for Haywire (S5L8747X) - Apple Lightning video adapters

You can run it on iOS as well (if you are lucky), on older versions it might take up to 1-1.5 minutes

## SoC support

Only **S5L8747X** - Haywire SoC, obviously

## Usage

```
➜  haywire_checkm8 git:(master) ✗ build/haywire_checkm8                                                                      
***      checkm8 exploit by @axi0mX      ***
*** s5l8747x implementaion by @a1exdandy ***
*** libirecovery/iOS port by @nyan_satan ***

found: CPID:8747 CPRV:10 CPFM:03 SCEP:10 BDID:00 ECID:REDACTED IBFL:00 SRTG:[iBoot-1413.8]
leaking...
triggering UaF...
sending payload...
found: CPID:8747 CPRV:10 CPFM:03 SCEP:10 BDID:00 ECID:REDACTED IBFL:00 SRTG:[iBoot-1413.8] PWND:[checkm8]
exploit success!
```

## Building

Requirements:

* [lilirecovery](https://github.com/NyanSatan/lilirecovery)
    * My little libirecovery fork
    * Included as a Git module

Then just use `make`:

```
➜  haywire_checkm8 git:(master) ✗ make                                                                                       
        building haywire_checkm8 for iOS
        building haywire_checkm8 for Mac
done!
```

## Credits

* **@axi0mX** - for checkm8 bug & original exploit
* **@a1exdandy** - for original Haywire port
* People behind **libimobiledevice** project - for libirecovery
