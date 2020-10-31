LAMBDA_ADDRESS = "SOMETHING.aws.REGION.amazonaws.com"
HISTORY_WINDOW = 360       // Number of seconds of lightning history to show

// setInterval(function() {
//     fetch(LAMBDA_ADDRESS + "/update/" + HISTORY_WINDOW)
//         .then(response => {
//             data = response.json();
//             // TODO: Do something with this json block. Render it
//         })
// }, 1000);


// var update_url = LAMBDA_ADDRESS + "/update"
var update_url = "sample_data.json"     // Temporary stand-in

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
    collection.lightning.forEach(function(d) {
        d.LatLng = new L.LatLng(d.coordinates[0],
                                d.coordinates[1])
    })

    var node_feature = g.selectAll("node_circles")
        .data(collection.nodes)
        .enter().append("circle")
        .style("stroke", "black")
        .style("opacity", .6)
        .style("fill", "red")
        .attr("r", 15);

    var lightn_feature = g.selectAll("lightn_circles")
        .data(collection.lightning)
        .enter().append("circle")
        .style("stroke", "black")
        .style("opacity", 1)
        .style("fill", "yellow")
        .attr("r", 5);

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

        lightn_feature.attr("transform",
        function(d) {
            return "translate("+
                map.latLngToLayerPoint(d.LatLng).x +","+
                map.latLngToLayerPoint(d.LatLng).y +")";
            }
        )
    }
})