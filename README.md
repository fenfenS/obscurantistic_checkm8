# obscurantistic_checkm8

Little rewrite of checkm8 in C for obscure targets that don't have a solution working on modern \*OS, such as Apple Lightning video adapters and Apple TV 3 (2013)

You can run it on iOS as well (if you are lucky)

On older versions of iOS & OS X it might take up to 1-1.5 minutes and even more

## SoC support

* **S5L8747X** - Haywire
* **S5L8947X** - A5 (single-core)

## Usage

```
➜  obscurantistic_checkm8 git:(master) ✗ build/obscurantistic_checkm8
***          checkm8 exploit by @axi0mX           ***
*** s5l8747x/Haywire implementation by @a1exdandy ***
***     libirecovery/iOS port by @nyan_satan      ***

found: CPID:8947 CPRV:00 CPFM:03 SCEP:10 BDID:00 ECID:REDACTED IBFL:00 SRTG:[iBoot-1458.2]
leaking...
triggering UaF...
sending payload...
found: CPID:8947 CPRV:00 CPFM:03 SCEP:10 BDID:00 ECID:REDACTED IBFL:00 SRTG:[iBoot-1458.2] PWND:[checkm8]
exploit success!
took - 1.66s
```

## Building

Requirements:

* [lilirecovery](https://github.com/NyanSatan/lilirecovery)
    * My little libirecovery fork
    * Included as a Git module

Then just use `make`:

```
➜  obscurantistic_checkm8 git:(master) ✗ make                                                                                       
        building obscurantistic_checkm8 for iOS
        building obscurantistic_checkm8 for Mac
done!
```

## Credits

* **@axi0mX** - for checkm8 bug & original exploit
* **@a1exdandy** - for original Haywire port
* People behind **libimobiledevice** project - for libirecovery
