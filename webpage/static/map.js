LAMBDA_ADDRESS = "https://uo4o2d00h5.execute-api.us-east-2.amazonaws.com/default/lambda_rest"
HISTORY_WINDOW = 360       // Number of seconds of lightning history to show

// setInterval(function() {
//     fetch(LAMBDA_ADDRESS + "/update/" + HISTORY_WINDOW)
//         .then(response => {
//             data = response.json();
//             // TODO: Do something with this json block. Render it
//         })
// }, 1000);


// var update_url = LAMBDA_ADDRESS;
// var update_url = "sample_data.json"  // Temporary stand-in

var map = L.map('map').setView([38.638336, -90.1], 11);
mapLink =
    '<a href="http://openstreetmap.org">OpenStreetMap</a>';
L.tileLayer(
    'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; ' + mapLink + ' Contributors',
    maxZoom: 17,
    }).addTo(map);

/* Initialize the SVG layer */
map._initPathRoot();

$(function(){
    $('#datetime_from').combodate();
    $('#datetime_to').combodate();
});

/* We simply pick up the SVG from the map object */
var svg = d3.select("#map").select("svg"),
g = svg.append("g");

let nodeClicked = -1;
let nodeNameCliked = null;
var lightn_feature = null;

// getData();
// setInterval(getData, 3000);


// function getData() {
var currDateTimeNode = Math.round((new Date().getTime())/1000);
console.log(currDateTimeNode);
d3.json("http://127.0.0.1:5000/update/1507600000/" + currDateTimeNode, function(collection) {
    console.log(collection);
    /* Add a LatLng object to each item in the dataset */
    collection.nodes.forEach(function(d) {
        d.LatLng = new L.LatLng(d.coordinates[0],
                                d.coordinates[1])
    })

    if (collection.lightning != null) {
        collection.lightning.forEach(function(d) {
            d.LatLng = new L.LatLng(d.coordinates[0],
                d.coordinates[1])
        })
    }

    var node_feature = g.selectAll("node_circles")
        .data(collection.nodes)
        .enter().append("circle")
        .style("stroke", "black")
        .style("opacity", .8)
        .style("fill", "#67c8c7")
        .attr("r", 15)
        .on('click', function(d) {
            d3.select('.columnLeft').style('width','50%');
            d3.select('.columnRight').style('width','50%');
            d3.select('.columnLeft').style('transition','width 0.5s');
            d3.select('.columnRight').style('transition','width 0.5s');

            if (d.nodename == "node01") {
                d3.select('#node_1').style('visibility','visible');
                d3.select('#node').style('visibility','hidden');

                d3.select('#node_1_coordinates').text("[" + d.coordinates[0] + ", " + d.coordinates[1] + "]");
                d3.select('#node_1_time').text(d.timestamp);
                d3.select('#node_1_battery').text(d.battery);
                d3.select('#node_1_temp').text(d.temp);
                d3.select('#node_1_pressure').text(d.pressure);
                d3.select('#node_1_humidity').text(d.humidity);
                d3.select('#node_1_rainfall').text(d.rainfall);
                d3.select('#node_1_wind_direction').text(d.wind_direction);
                d3.select('#node_1_wind_speed').text(d.wind_speed);

                d3.select('.chart').style('visibility','visible');
                nodeNameCliked = d.nodename;
            } else {
                d3.select('#node_1').style('visibility','hidden');
                d3.select('#node').style('visibility','visible');

                d3.select('#node_coordinates').text("[" + d.coordinates[0] + ", " + d.coordinates[1] + "]");
                d3.select('#node_time').text(d.timestamp);
                d3.select('#node_battery').text(d.battery);

                d3.select('.chart').style('visibility','hidden');
            }
            d3.select('#info-header').text(d.nodename);
            d3.select('#Refresh').style('visibility','visible');
            d3.select('#node_config').style('visibility','visible');
            d3.select('#light').style('visibility','hidden');

            collection.nodes.find(function(item, i){
                if(item.nodename === d.nodename){
                    nodeClicked = i;
                }
            });
        });
            
    if (collection.lightning != null) {
        lightn_feature = g.selectAll("lightn_circles")
        .data(collection.lightning)
        .enter().append("circle")
        .style("stroke", "black")
        .style("opacity", 0.6)
        .style("fill", "#ffcc00")
        .attr("r", 10)
        .on('click', function(d) {
            d3.select('.columnLeft').style('width','50%');
            d3.select('.columnRight').style('width','50%');
            d3.select('.columnLeft').style('transition','width 0.5s');
            d3.select('.columnRight').style('transition','width 0.5s');

            d3.select('#node_1').style('visibility','hidden');
            d3.select('#node').style('visibility','hidden');
            d3.select('#light').style('visibility','visible');
            d3.select('.chart').style('visibility','hidden');
            d3.select('#Refresh').style('visibility','hidden');
            d3.select('#node_config').style('visibility','hidden');

            d3.select('#light_coordinates').text("[" + d.coordinates[0] + ", " + d.coordinates[1] + "]");
            d3.select('#light_time').text(d.timestamp);

            d3.select('#info-header').text("Lightning Info");
        });
    }

    map.on("viewreset", update);
    update();

    function update() {
        node_feature.attr("transform",
        function(d) {
            return "translate("+
                map.latLngToLayerPoint(d.LatLng).x +","+
                map.latLngToLayerPoint(d.LatLng).y +")";
            }
        )

        if (lightn_feature != null) {
            lightn_feature.attr("transform", function(d) {
                return "translate("+
                    map.latLngToLayerPoint(d.LatLng).x +","+
                    map.latLngToLayerPoint(d.LatLng).y +")";
                }
            )
        }
    }
})

document.getElementById("Refresh").onclick = function() {getRefresh()};
function getRefresh() {
    if (nodeClicked >= 0) {
        console.log("clicked " + nodeClicked);
        var currDateTimeTable = Math.round((new Date().getTime())/1000);
        $.get( "http://127.0.0.1:5000/update/1507600000/" + currDateTimeTable, function( data ) {
            const nodeShow = data.nodes[nodeClicked];
            if (nodeShow.nodename =="node01" && document.getElementById("node_1").style.visibility == "visible") {
                document.getElementById('node_1_time').textContent = nodeShow.timestamp;
                document.getElementById('node_1_battery').textContent = nodeShow.battery;
                document.getElementById('node_1_temp').textContent = nodeShow.temp;
                document.getElementById('node_1_pressure').textContent = nodeShow.pressure;
                document.getElementById('node_1_humidity').textContent = nodeShow.humidity;
                document.getElementById('node_1_rainfall').textContent = nodeShow.rainfall;
                document.getElementById('node_1_wind_direction').textContent = nodeShow.wind_direction;
                document.getElementById('node_1_wind_speed').textContent = nodeShow.wind_speed;
            } else if (nodeShow.nodename !="node01" && document.getElementById("node").style.visibility == "visible") {
                document.getElementById('node_time').textContent = nodeShow.time;
                document.getElementById('node_battery').textContent = nodeShow.battery;
            }
        }, "json" );
    }
}

/************** CHART *****************/
var historic_url = "historic_data.json"  // Temporary stand-in

var margin = {top: 15, right: 50, bottom: 100, left: 50}
, width = 350 // Use the window's width 
, height = 175;
// , width = window.innerWidth - margin.left - margin.right // Use the window's width 
// , height = window.innerHeight - margin.top - margin.bottom; // Use the window's height

document.getElementById("form_button").onclick = function() {getChart()};
function getChart() {
    var dateFrom = document.getElementById('FromDate').value;
    var timeFrom = document.getElementById('datetime_from').value;
    var valFrom =new Date(dateFrom+"T"+timeFrom+":00Z");

    var dateTo= document.getElementById('ToDate').value;
    var timeTo = document.getElementById('datetime_to').value;
    var valTo =new Date(dateTo+"T"+timeTo+":00Z");

    if (valFrom >= valTo) {
        alert("Invalid Input");
    } else {
        if (nodeNameCliked == null) {
          alert("No Node Clicked");
          return;
        }

        $("#pressure").empty();
        $("#temprature").empty(); 
        $("#humidity").empty();
        $("#rainfall").empty();

        $.get( "http://127.0.0.1:5000/history/" + nodeNameCliked  + "/" + Math.round(valFrom.getTime()/1000) + "/" + Math.round(valTo.getTime()/1000), function( data ) {
            console.log(data);
            const n = data.history.length;
            const history = data.history;
        
            let max = 0;
            let min = new Date(history[0].timestamp).getTime();
            let minPressure = history[0].pressure;
            let maxPressure = 0;
            let minTemprature = history[0].temp;
            let maxTemprature = 0;
            let minHumidity = history[0].humidity;
            let maxHumidity = 0;
            let maxRainFall = 0;
        
            let pressure = [];
            let temprature = [];
            let humidity = [];
            let rainfall = [["rainfall 1h", data.rainfall_1h], ["rainfall 6h", data.rainfall_6h], ["rainfall 24h", data.rainfall_24h]];
            for (i = 0; i<n; i++) {
                const date = new Date(history[i].timestamp);
                const time = date.getTime();
                if (time > max) max = time;
                else if (time < min) min = time;
        
                maxPressure = Math.max(maxPressure, history[i].pressure);
                minPressure = Math.min(minPressure, history[i].pressure);
        
                maxTemprature = Math.max(maxTemprature, history[i].temp);
                minTemprature = Math.min(minTemprature, history[i].temp);
        
                maxHumidity = Math.max(maxHumidity, history[i].humidity);
                minHumidity = Math.min(minHumidity, history[i].humidity);
        
                pressure[i] = [time, history[i].pressure];
                temprature[i] = [time, history[i].temp];
                humidity[i] = [time, history[i].humidity];
            }
        
            maxRainFall = Math.max(Math.max(Math.max(rainfall[0][1], maxRainFall), rainfall[1][1]), rainfall[2][1]);
            if (maxRainFall == 0) maxRainFall = 20;
            console.log(maxHumidity, minHumidity);
            var minDate = new Date,
                maxDate = new Date;
            
            minDate.setTime(min);
            maxDate.setTime(max);
        
            var xScale = d3.time.scale()
                .domain([new Date(history[0].timestamp), new Date(history[n-1].timestamp)]) // input
                .range([0, width]); // output
                
            /*****Pressure */
            var yScaleP = d3.scaleLinear()
                .domain([minPressure, maxPressure]) // input 
                .range([height, 0]); // output
        
            var lineP = d3.line()
                .x(function(d) {
                    var tempDate = new Date;
                    tempDate.setTime(d[0]);
                    return xScale(tempDate);
                }) // set the x values for the line generator
                .y(function(d) { return yScaleP(d[1]); }) // set the y values for the line generator 
        
            // 1. Add the SVG to the page and employ #2
            var svgP = d3.select("#pressure").append("svg")
            .attr("width", width + margin.left + margin.right)
            .attr("height", height + margin.top + margin.bottom)
            .append("g")
            .attr("transform", "translate(" + margin.left + "," + margin.top + ")");
        
            // 3. Call the x axis in a group tag
            svgP.append("g")
            .attr("class", "x axis")
            .attr("transform", "translate(0," + height + ")")
            .call(d3.axisBottom(xScale).tickFormat(d3.timeFormat("%Y-%m-%d %H:%M:%S"))) // Create an axis component with d3.axisBottom
            .selectAll("text")	
                .style("text-anchor", "end")
                .attr("dx", "-.8em")
                .attr("dy", ".15em")
                .attr("transform", "rotate(-65)");
        
            // 4. Call the y axis in a group tag
            svgP.append("g")
            .attr("class", "y axis")
            .call(d3.axisLeft(yScaleP)); // Create an axis component with d3.axisLeft
        
            // 9. Append the path, bind the data, and call the line generator 
            svgP.append("path")
            .datum(pressure) // 10. Binds data to the line 
            .attr("class", "line") // Assign a class for styling 
            .attr("d", lineP); // 11. Calls the line generator 
        
            svgP.append("text")
                .attr("x", (width / 2))             
                .attr("y", 0 - (margin.top / 12))
                .attr("text-anchor", "middle")  
                .style("font-size", "13px") 
                .text("Pressure");
        
        
            /*****Temprature */
            var yScaleT = d3.scaleLinear()
            .domain([minTemprature, maxTemprature]) // input 
            .range([height, 0]); // output
        
            var lineT = d3.line()
                .x(function(d) {
                    var tempDate = new Date;
                    tempDate.setTime(d[0]);
                    return xScale(tempDate);
                }) // set the x values for the line generator
                .y(function(d) { return yScaleT(d[1]); }) // set the y values for the line generator 
        
            var svgT = d3.select("#temprature").append("svg")
            .attr("width", width + margin.left + margin.right)
            .attr("height", height + margin.top + margin.bottom)
            .append("g")
            .attr("transform", "translate(" + margin.left + "," + margin.top + ")");
        
            svgT.append("g")
            .attr("class", "x axis")
            .attr("transform", "translate(0," + height + ")")
            .call(d3.axisBottom(xScale).tickFormat(d3.timeFormat("%Y-%m-%d %H:%M:%S"))) // Create an axis component with d3.axisBottom
            .selectAll("text")	
                .style("text-anchor", "end")
                .attr("dx", "-.8em")
                .attr("dy", ".15em")
                .attr("transform", "rotate(-65)");
        
            svgT.append("g")
            .attr("class", "y axis")
            .call(d3.axisLeft(yScaleT)); // Create an axis component with d3.axisLeft
        
            svgT.append("path")
            .datum(temprature) // 10. Binds data to the line 
            .attr("class", "line") // Assign a class for styling 
            .attr("d", lineT); // 11. Calls the line generator 
        
            svgT.append("text")
                .attr("x", (width / 2))             
                .attr("y", 0 - (margin.top / 12))
                .attr("text-anchor", "middle")  
                .style("font-size", "13px") 
                .text("Temprature");
        
            /*****Humidity */
            var yScaleH = d3.scaleLinear()
                .domain([minHumidity, maxHumidity]) // input 
                .range([height, 0]); // output
        
            var lineH = d3.line()
                .x(function(d) {
                    var tempDate = new Date;
                    tempDate.setTime(d[0]);
                    return xScale(tempDate);
                }) // set the x values for the line generator
                .y(function(d) { return yScaleH(d[1]); }) // set the y values for the line generator 
        
            var svgH = d3.select("#humidity").append("svg")
            .attr("width", width + margin.left + margin.right)
            .attr("height", height + margin.top + margin.bottom)
            .append("g")
            .attr("transform", "translate(" + margin.left + "," + margin.top + ")");
        
            svgH.append("g")
            .attr("class", "x axis")
            .attr("transform", "translate(0," + height + ")")
            .call(d3.axisBottom(xScale).tickFormat(d3.timeFormat("%Y-%m-%d %H:%M:%S"))) // Create an axis component with d3.axisBottom
            .selectAll("text")	
                .style("text-anchor", "end")
                .attr("dx", "-.8em")
                .attr("dy", ".15em")
                .attr("transform", "rotate(-65)");
        
            svgH.append("g")
            .attr("class", "y axis")
            .call(d3.axisLeft(yScaleH)); // Create an axis component with d3.axisLeft
        
            svgH.append("path")
            .datum(humidity) // 10. Binds data to the line 
            .attr("class", "line") // Assign a class for styling 
            .attr("d", lineH); // 11. Calls the line generator 
        
            // svgH.selectAll(".dot")
            // .data(humidity)
            // .enter().append("circle") // Uses the enter().append() method
            // .attr("class", "dot") // Assign a class for styling
            // .attr("cx", function(d) { 
            //     var tempDate = new Date;
            //     tempDate.setTime(d[0]);
            //     return xScale(tempDate); 
            // })
            // .attr("cy", function(d) { return yScaleH(d[1]) })
            // .attr("r", 5);
        
            svgH.append("text")
                .attr("x", (width / 2))             
                .attr("y", 0 - (margin.top / 12))
                .attr("text-anchor", "middle")  
                .style("font-size", "13px") 
                .text("Humidity");
        
            /*******Rainfall */
            var xR = d3.scaleBand()
                .range([0, width])
                .padding(0.6);
        
            var yR = d3.scaleLinear()
                    .range([height, 0]);
                    
            // append the svg object to the body of the page
            // append a 'group' element to 'svg'
            // moves the 'group' element to the top left margin
            var svgR = d3.select("#rainfall").append("svg")
                .attr("width", width + margin.left + margin.right)
                .attr("height", height + margin.top + margin.bottom)
            .append("g")
                .attr("transform", 
                    "translate(" + margin.left + "," + margin.top + ")");
        
            // Scale the range of the data in the domains
            xR.domain(rainfall.map(function(d) { return d[0]; }));
            yR.domain([0, maxRainFall]);
        
            // append the rectangles for the bar chart
            svgR.selectAll(".bar")
                .data(rainfall)
                .enter().append("rect")
                .attr("class", "bar")
                .attr("x", function(d) { return xR(d[0]); })
                .attr("width", xR.bandwidth())
                .attr("y", function(d) { return yR(d[1]); })
                .attr("height", function(d) { return height - yR(d[1]); });
        
            // add the x Axis
            svgR.append("g")
                .attr("transform", "translate(0," + height + ")")
                .call(d3.axisBottom(xR));
        
            // add the y Axis
            svgR.append("g")
                .call(d3.axisLeft(yR));
        
            svgR.append("text")
                .attr("x", (width / 2))             
                .attr("y", 0 - (margin.top / 12))
                .attr("text-anchor", "middle")  
                .style("font-size", "13px") 
                .text("Rainfall");
        }, "json" );
    }
}