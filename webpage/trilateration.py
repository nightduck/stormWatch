import math
import numpy

#assuming elevation = 0
earthR = 6371
lat_A = 37.418436
lon_A = -121.963477
d_A = 0.265710701754
lat_B = 37.417243
lon_B = -121.961889
d_B = 0.234592423446
lat_C = 37.418692
lon_C = -121.960194
d_C = 0.0548954278262

#using authalic sphere
#if using an ellipsoid this step is slightly different
#Convert geodetic Lat/Long to ECEF xyz
#   1. Convert Lat/Long to radians
#   2. Convert Lat/Long(radians) to ECEF
def convert_to_ECEF(lat, lon):
    x = earthR * (math.cos(math.radians(lat)) * math.cos(math.radians(lon)))
    y = earthR * (math.cos(math.radians(lat)) * math.sin(math.radians(lon)))
    z = earthR * (math.sin(math.radians(lat)))
    return x, y, z

xA, yA, zA = convert_to_ECEF(lat_A, lon_A)
xB, yB, zB = convert_to_ECEF(lat_B, lon_B)
xC, yC, zC = convert_to_ECEF(lat_C, lon_C)


P1 = numpy.array([xA, yA, zA])
P2 = numpy.array([xB, yB, zB])
P3 = numpy.array([xC, yC, zC])

# transform to get circle 1 at origin
# transform to get circle 2 on x axis
ex = (P2 - P1)/(numpy.linalg.norm(P2 - P1))
i = numpy.dot(ex, P3 - P1)
ey = (P3 - P1 - i*ex)/(numpy.linalg.norm(P3 - P1 - i*ex))
ez = numpy.cross(ex, ey)
d = numpy.linalg.norm(P2 - P1)
j = numpy.dot(ey, P3 - P1)

# plug and chug using above values
x = (pow(d_A, 2) - pow(d_B, 2) + pow(d, 2)) / (2 * d)
y = ((pow(d_A, 2) - pow(d_C, 2) + pow(i, 2) + pow(j, 2)) / (2 * j)) - ((i / j) * x)

# only one case shown here
z = numpy.sqrt(pow(d_A, 2) - pow(x, 2) - pow(y, 2))

# triPt is an array with ECEF x,y,z of trilateration point
triPt = P1 + x*ex + y*ey + z*ez

# convert back to lat/long from ECEF
# convert to degrees
lat = math.degrees(math.asin(triPt[2] / earthR))
lon = math.degrees(math.atan2(triPt[1], triPt[0]))

print(lat, lon)

