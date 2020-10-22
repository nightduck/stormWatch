This project was built in VS Code's PlatformIO extension. See intro video [here](https://www.youtube.com/watch?v=dany7ae_0ks). The step where you download the Arduino IDE doesn't seem to be neccessary on MacOS.

## Cloning

    git clone https://github.com/nightduck/stormWatch.git
    cd stormWatch
    git update-index --assume-unchanged data/config

The last line prevents you from accidentally publishing your config, and all the private keys associated with it, to the git tree.