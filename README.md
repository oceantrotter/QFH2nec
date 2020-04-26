# Helix2nec and QFH2nec

This repository hosts software that helps generates NEC files for QFH antennas.

Helix2nec is a straight copy from the code provided here: https://uuki.kapsi.fi/qha_simul.html

QFH2nec has been adapted to include code from this calculator: https://www.jcoppens.com/ant/qfh/calc.en.php

Both software can be compiled with GCC and no external libraries (apart from the standard C libraries) with ` make all `.



## Usage

Helix2nec uses a specific file for input and can generate a lot of helix antennas within the same file. Please see the documentation linked above.

QFH2nec is called with this:
` QFH2nec <Design frequency in MHz> <Number of turns> <Length of one turn in wavelengths> <Bending radius> <Conductor diameter> <Width/height ratio>`

The generated NEC files can then be opened with xnec2c for example. xnec2c can be downloaded from https://www.qsl.net/5/5b4az/, Ham Radio Software -> Antenna Software.


## License: GPLv3
The original Helix2nec software is provided as is without support of any kind.
If the original author doesn't want it provided here, please let me know.
The modified Helix2nec code in QFH2nec is provided under a GPLv3 license. Also, if the original author doesn't want it here, please let me know.

## TODO
- [ ] More documentation on usage
- [ ] Rewrite some parts to center the helixes of the antenna itself
- [ ] Provide examples of data input
- [ ] Rewrite command line argument parsing to a more user-friendly way
