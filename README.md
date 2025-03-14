# Typc

Typc is a lightweight, console-based typing trainer written in C.

![preview](https://github.com/JoshAU-04/typc/blob/main/.github/assets/preview.gif?raw=true)

## About

Typc is a minimalistic, console-based typing trainer. It focuses on core
functionality without relying on complex command line parsing libraries.
Despite its simplicity, Typc offers several useful features to help improve
your typing speed and accuracy. For more information, see the [wiki](https://github.com/JoshAU-04/typc/wiki)

## Features

Typc provides a detailed analysis of your typing performance by comparing your
input to a [randomly selected text](https://github.com/JoshAU-04/typc/wiki/Randomization).
After completing a test, you will see your performance statistics on the [Results Page](https://github.com/JoshAU-04/typc/wiki/Results-Page).
If you'd like, you can also easily [add your own texts](https://github.com/JoshAU-04/typc/wiki/Adding-Texts#adding-new-texts).
Typc also has a few arguments that you can give it to customize how the program
behaves which are all detailed on the [wiki](https://github.com/JoshAU-04/typc/wiki).


## Usage

To use Typc, simply run the executable from the source directory. This is
important because the program relies on relative I/O paths. For example, the
`texts` directory must be located in the same directory where the executable is
run. Running Typc from a directory without the required structure (e.g.,
`~/Downloads/typc` without a `texts` folder) will result in errors.

> [!NOTE]
> This only applies when running from source. If you've run `sudo make install`
> or simply run `sudo ./INSTALL` then this doesn't really apply at all.


## License

This project is licensed under GPLv3. See [LICENSE](https://github.com/JoshAU-04/typc/blob/main/LICENSE) for the full license text.
