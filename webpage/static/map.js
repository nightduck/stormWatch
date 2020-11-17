LAMBDA_ADDRESS = "https://uo4o2d00h5.execute-api.us-east-2.amazonaws.com/default/lambda_rest"
HISTORY_WINDOW = 360       // Number of seconds of lightning history to show

// setInterval(function() {
//     fetch(LAMBDA_ADDRESS + "/update/" + HISTORY_WINDOW)
//         .then(response => {
//             data = response.json();
//             // TODO: Do something with this json block. Render it
//         })
// }, 1000);


var update_url = LAMBDA_ADDRESS;
// var update_url = "sample_data.json"  // Temporary stand-in

var map = L.map('map').setView([38.638336, -90.284672], 11);
mapLink =
    '<a href="http://openstreetmap.org">OpenStreetMap</a>';
L.tileLayer(
    'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; ' + mapLink + ' Contributors',
    maxZoom: 18,
    }).addTo(map);

/* Initialize the SVG layer */
map._initPathRoot()

/* We simply pick up the SVG from the map object */
var svg = d3.select("#map").select("svg"),
g = svg.append("g");

d3.json(update_url, function(collection) {
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
        .style("opacity", .6)
        .style("fill", "red")
        .attr("r", 15)
        .on('click', function(d) {
            d3.select('#node').style('visibility','visible');
            d3.select('#light').style('visibility','hidden');
            d3.select('#info-header').style('visibility','visible');
            d3.select('#node_nodename').text("nodename: " + d.nodename + "; ");
            d3.select('#node_coordinates').text("coordinates: [" + d.coordinates[0] + ", " + d.coordinates[1] + "]; ");
            d3.select('#node_temp').text("temp: " + d.temp + "; ");
            d3.select('#node_pressure').text("pressure: " + d.pressure + "; ");
            d3.select('#node_humidity').text("humidity: " + d.humidity + "; ");
            d3.select('#node_rainfall').text("rainfall: " + d.rainfall + "; ");
            d3.select('#node_wind_direction').text("wind_direction: " + d.wind_direction + "; ");
            d3.select('#node_wind_speed').text("wind_speed: " + d.wind_speed + "; ");
        });
            

    var lightn_feature = null;
    if (collection.lightning != null) {
        lightn_feature = g.selectAll("lightn_circles")
         .data(collection.lightning)
         .enter().append("circle")
         .style("stroke", "black")
         .style("opacity", 1)
         .style("fill", "yellow")
         .attr("r", 10)
         .on('click', function(d) {
             d3.select('#info-header').style('visibility','visible');
             d3.select('#light').style('visibility','visible');
             d3.select('#node').style('visibility','hidden');
             d3.select('#light_coordinates').text("coordinates: [" + d.coordinates[0] + ", " + d.coordinates[1] + "]; ");
             d3.select('#light_time').text("time: " + d.time + "; ")
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