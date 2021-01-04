This project was built in VS Code's PlatformIO extension. See intro video [here](https://www.youtube.com/watch?v=dany7ae_0ks). The step where you download the Arduino IDE doesn't seem to be neccessary on MacOS.

## Cloning

    git clone https://github.com/nightduck/stormWatch.git
    cd stormWatch
    git update-index --assume-unchanged data/config.json

The last line prevents you from accidentally publishing your config, and all the private keys associated with it, to the git tree.

## Website

To test and run the website locally on your device, you can simply navigate to this folder and in a terminal,

    python app.py

Then, copy the link from the terminal and paste to a web browser (Google Chrome prefered).

Make sure to use the right link for Lambda REST call in tormWatch/webpage/static/map.js

### Weather Monitoring and Lightning Visualization

The locations of deployed weather nodes are marked with red circles. The detected lightning strikes are marked with yellow circles. Click any one of them for detailed information, such as observation time and coordinates. If a weather node is selected, the most recent sensor readings will appear on the right side of the map. Click the "Refresh" button to refresh the data.

### History Weather Data

If the full node is selected, on the bottom of the sensor value table, you can set a start and stop time to fetch the history weather data in this time frame. The website will plot the history temperature, humidity, pressure, and rain percipitation.

### Remote Configuration

If a weather node is selected, at the top right corner of the webpage, you can set up the values of the key parameters of that weather node. By clicking the "Deploy" button to confirm the change.

