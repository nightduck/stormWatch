This project was built in VS Code's PlatformIO extension. See intro video [here](https://www.youtube.com/watch?v=dany7ae_0ks). The step where you download the Arduino IDE doesn't seem to be neccessary on MacOS.

## Cloning

    git clone https://github.com/nightduck/stormWatch.git
    cd stormWatch
    git update-index --assume-unchanged data/config.json

The last line prevents you from accidentally publishing your config, and all the private keys associated with it, to the git tree.

## Website

To test and run the website locally on your device, you can simply navigate to this folder and

    python app.py

Make sure to use the right link for Lambda REST call in tormWatch/webpage/static/map.js