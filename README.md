nRF52840 Dongle Dumper
======================

This code dumps the flash, FICR, and UICR from an nRF52840 Dongle.

Motivation
----------
By default, nRF52840 revision F and later has hardware/software APPROTECT that
disables SWD access even if the hardware has it opened, until the software
explicitly opens access. The bootloader on current nRF52840 dongles does not
do that. So to dump the initial state of such a dongle, the goal is to write
as small of an application to exfiltrate the content of the flash. The page
size of the flash is 4KB, so the code has to fit under that to avoid erasing
more flash than necessary. The code should also attempt some level of
compression to avoid having to dump out a whole 1MB of data to dump the flash.

Building
--------
This project is designed only to be used with the GNU Arm Embedded Toolchain.
Please set that up in your nRF5 SDK. Then update the SDK path in
`pca10059/mbr/armgcc/Makefile` under `SDK_ROOT`. Then simply run `make` in the
directory of the `Makefile`. You can flash the resulting `.hex` file within the
`_build` folder using the Programmer in nRF Connect for Desktop.

You should also build Shrink (included as submodule) to decompress the data.

Usage
-----
Connect a UART adapter to the board, where the board's TX is pin `0.15`. Capture
the output from the dongle (about 200KB is good). Once you've completed the
capture, open it and search for the start magic bytes. The following outlines
the format of the output:

- `55 aa d0 0d`: start magic number
- starting address, 32-bit little-endian
- uncompressed length, 32-bit little-endian
- RLE compressed data
- `0d d0 aa 55`: end magic number

Extract the compressed main flash data to a separate file. Note that when
searching for the magic numbers, they may appear early because they are
constants within the code and not very compressible. So skip past the constant
as necessary. Repeat extraction with data after that for FICR and UICR. Use
Shrink like this to decompress:

```
shrink -r -d dumped.bin decompressed.bin
```

Analysis
--------
For the most part, the main flash is blank. It contains an MBR and an Open
Bootloader with DFU, plus the settings for the bootloader, but is otherwise
blank. For differences with the original flash data, you may find the dumper
code plus (this is conjecture) a slightly modified bootloader settings area that
has registered the dumper code that you have flashed.

In the UICR, there is `NRFFW[0]` and `NRFFW[1]` filled out, plus additional
options `PSELRESET[0]` and `PSELRESET[1]` to specify the reset pin; `APPROTECT`
set to hardware disable of AP protection; and `REGOUT0` to 3.0V so it could be
programmed from an nRF52840 DK if you would like to solder an SWD programming
header on the back.

Note that the flash contents basically match the
[downloadable bootloader](https://devzone.nordicsemi.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-00-13/pca10059_5F00_bootloader.zip)
provided on the [nRF52840 Dongle Programming Tutorial](https://devzone.nordicsemi.com/guides/short-range-guides/b/getting-started/posts/nrf52840-dongle-programming-tutorial).
Note, however, because this tutorial was written before the chip's revision F,
it assumed that the AP protection was disabled and you could reflash using the
SWD port. This is no longer true for revision F and later chips, so if you
would like to debug some code, you may want to attempt to modify the MBR or
bootloader to disable AP protection in software, or at least add that to your
code if you are debugging your own code.

If you are reflashing the original bootloader, all of the mentioned registers
except `APPROTECT` should automatically be configured after the bootloader runs.
You should set `APPROTECT` separately while flashing.
