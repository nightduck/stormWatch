from flask import Flask
import json
import sys
import logging
import importlib
import pymysql
import datetime

import os

app = Flask(__name__)

path = os.getcwd()

cfg = json.load(open("rds_config.json", 'r'))
rds_host  = cfg["rds_host"]
name = cfg["username"]
password = cfg["password"]
db_name = cfg["db_name"]

logger = logging.getLogger()
logger.setLevel(logging.INFO)

try:
    conn = pymysql.connect(rds_host, user=name, passwd=password, db=db_name, connect_timeout=5)
except pymysql.MySQLError as e:
    logger.error("ERROR: Unexpected error: Could not connect to MySQL instance.")
    logger.error(e)
    sys.exit()
logger.info("SUCCESS: Connection to RDS MySQL instance succeeded")

@app.route('/')
def index():
    return app.send_static_file("index.html")

@app.route('/<path:path>')
def rsc_file(path):
    return app.send_static_file(path)


# Extract nodename, full_node from DB nodes
def extract_nodes():
  with conn.cursor() as cur:
    cur.execute("select nodename, full_node from stormwatch.nodes")
    conn.commit()
  data=cur.fetchall()
  nodes=[]
  for row in data:
    nodes.append([row[0], row[1]])
  return nodes
  

def helper_weather_extract(data, full_node):
  if full_node:
    temp = float(data[0]) if data[0]!=None else None
    humidity = float(data[1]) if data[1]!=None else None
    wind_direction=str(data[2]) if data[2]!=None else None
    wind_speed=float(data[3]) if data[3]!=None else None
    rainfall=float(data[4]) if data[4]!=None else None
    nodename=str(data[5]) 
    latitude=float(data[6]) 
    longitude=float(data[7]) 
    timestamp=str(data[8]) 
    pressure=float(data[9]) if data[9]!=None else None
    battery=float(data[10]) if data[10]!=None else None
    
    weather_data={
      "nodename" : nodename,
      "coordinates" : [longitude, latitude],
      "temp" : temp,
      "pressure" : pressure,
      "humidity": humidity,
      "rainfall" : rainfall,
      "wind_direction" : wind_direction,
      "wind_speed" : wind_speed,
      "timestamp": timestamp,
      "battery": battery
    }
  else:
    nodename=str(data[0]) 
    latitude=float(data[1]) 
    longitude=float(data[2]) 
    timestamp=str(data[3]) 
    battery=float(data[4]) if data[4]!=None else None

    weather_data={
      "nodename" : nodename,
      "coordinates" : [longitude, latitude],
      "timestamp": timestamp,
      "battery": battery
    }
   
  return weather_data
  
# Extract data from stormwatch.weather_readings
# Return the latest weather record in the time window
# Nothing will be returned if the data of the node are missing in this time window

def extract_weather_data(start_time, end_time):
  nodes=extract_nodes()
  #print(nodes)
  weather_records=[]
  
  print(nodes)
  for node in nodes:
    if node[1]==1:
      with conn.cursor() as cur:
        cur.execute("select temp, humidity, wind_direction, wind_speed, rainfall, nodename, latitude, longitude, timestamp, pressure, battery\
        from stormwatch.weather_readings where timestamp <= %s and timestamp >= %s and nodename=%s\
        order by timestamp desc limit 1", (end_time, start_time, node[0]))
        conn.commit()
    else:
      with conn.cursor() as cur:
        cur.execute("select nodename, latitude, longitude, timestamp, battery\
        from stormwatch.weather_readings where timestamp <= %s and timestamp >= %s and nodename=%s\
        order by timestamp desc limit 1", (end_time, start_time, node[0]))
        conn.commit()
      
    data=cur.fetchone()

    if data==None:
      continue

    else:
      weather_records.append(helper_weather_extract(data, node[1]))
              
  return weather_records



# Extract lightning data in the time window for rest function
# Return a list of lightning record in lightning_strikes
# Return empty list if no data exists
def extract_lightning_data(lightning_start_time, lightning_end_time):
  with conn.cursor() as cur:
    cur.execute("select longitude, latitude, timestamp from stormwatch.lightning_strikes \
    where timestamp>=%s and timestamp<=%s",(lightning_start_time, lightning_end_time))
    conn.commit()
  data=cur.fetchall()
  lightning_data_list=[]

  #print(data)
  if data==():
    return lightning_data_list
    

  for row in data:
    longitude = float(row[0]) 
    latitude = float(row[1]) 
    timestamp=str(row[2]) 
    
    current_data={
      "coordinates" : [longitude, latitude],
      "timestamp": timestamp
      }
    
    lightning_data_list.append(current_data)
            
  return lightning_data_list



@app.route('/update/<t1>/<t2>')     # Arguments are integers: seconds since epoch
def update(t1, t2):
    start_time=datetime.datetime.fromtimestamp(int(t1)).strftime('%Y-%m-%d %H:%M:%S')
    end_time=datetime.datetime.fromtimestamp(int(t2)).strftime('%Y-%m-%d %H:%M:%S')

    
    nodes_data={
      "nodes":extract_weather_data(start_time, end_time),
      "lightning" : extract_lightning_data(start_time, end_time)
    }
   
    # Insert rest of code here from lambda_test
    #return '{"key":"value", "hey": "it works"}'
    #return {start_time}
    return nodes_data



def extract_history_weather_data(node, weather_start_time, weather_end_time):
  with conn.cursor() as cur:
    cur.execute("select full_node from stormwatch.nodes where nodename=%s",(node))
    conn.commit()
  full_node=cur.fetchone()[0]

 # History data for partial node
  if full_node==0:
    with conn.cursor() as cur:
      cur.execute("select longitude, latitude, timestamp, battery from stormwatch.weather_readings \
      where nodename=%s and timestamp>=%s and timestamp<=%s",(node, weather_start_time, weather_end_time))
      conn.commit()
    data=cur.fetchall()

    if data==():
      return {}
    
    longitude = float(data[0][0]) 
    latitude = float(data[0][1]) 
    
    weather_data_list=[]
    for row in data:
      timestamp=str(row[2]) if row[2]!=None else None
      battery=float(row[3]) if row[3]!=None else None
      
      current_data={
        "timestamp": timestamp,
        "battery": battery
        }
      
      weather_data_list.append(current_data)
              
    weather_data={
      "nodename" : node,
      "coordinates" : [longitude, latitude],
      "history": weather_data_list
      }
    return weather_data


  # History data with full node
  with conn.cursor() as cur:
    cur.execute("select longitude, latitude, temp, pressure, humidity, rainfall, wind_direction, wind_speed, timestamp, battery from stormwatch.weather_readings \
    where nodename=%s and timestamp>=%s and timestamp<=%s",(node, weather_start_time, weather_end_time))
    conn.commit()
  data=cur.fetchall()
  #print(data)

  if data==():
    return {}

  # Compute the start time of the rainfall window 
  rainfall_start_time_1h = datetime.datetime.strptime(weather_end_time, '%Y-%m-%d %H:%M:%S') - datetime.timedelta(hours=1)
  rainfall_start_time_6h = datetime.datetime.strptime(weather_end_time, '%Y-%m-%d %H:%M:%S') - datetime.timedelta(hours=6)
  rainfall_start_time_24h = datetime.datetime.strptime(weather_end_time, '%Y-%m-%d %H:%M:%S') - datetime.timedelta(hours=24)
  
  rainfall_start_time_1h=datetime.datetime.strftime(rainfall_start_time_1h, '%Y-%m-%d %H:%M:%S')
  rainfall_start_time_6h=datetime.datetime.strftime(rainfall_start_time_6h, '%Y-%m-%d %H:%M:%S')
  rainfall_start_time_24h=datetime.datetime.strftime(rainfall_start_time_24h, '%Y-%m-%d %H:%M:%S')

  
  with conn.cursor() as cur:
    cur.execute("select rainfall, timestamp from stormwatch.weather_readings where nodename=%s and timestamp>=%s and timestamp<=%s",(node, rainfall_start_time_24h, weather_end_time))
    conn.commit()
  data_rainfall_24h=cur.fetchall()
  
  rainfall_1h=0
  rainfall_6h=0
  rainfall_24h=0
  
  for row in data_rainfall_24h:
    if str(row[1])>rainfall_start_time_1h:
      rainfall_1h+=float(row[0])
    if str(row[1])>rainfall_start_time_6h:
      rainfall_6h+=float(row[0])
    rainfall_24h+=float(row[0])
  
    
  
  longitude = float(data[0][0]) 
  latitude = float(data[0][1]) 
  
  weather_data_list=[]
  for row in data:
    temp=float(row[2]) if row[2]!=None else None
    pressure=float(row[3]) if row[3]!=None else None
    humidity=float(row[4]) if row[4]!=None else None
    rainfall=float(row[5]) if row[5]!=None else None
    wind_direction=str(row[6]) if row[6]!=None else None
    wind_speed=float(row[7]) if row[7]!=None else None
    timestamp=str(row[8]) if row[8]!=None else None
    battery=float(row[9]) if row[9]!=None else None
    
    current_data={
      "temp" : temp,
      "pressure" : pressure,
      "humidity": humidity,
      "rainfall" : rainfall,
      "wind_direction" : wind_direction,
      "wind_speed" : wind_speed,
      "timestamp": timestamp,
      "battery": battery
      }
    
    weather_data_list.append(current_data)
            
  weather_data={
    "nodename" : node,
    "coordinates" : [longitude, latitude],
    "history": weather_data_list,
    "rainfall_1h": rainfall_1h,
    "rainfall_6h": rainfall_6h,
    "rainfall_24h": rainfall_24h
    }
  return weather_data

@app.route('/history/<node>/<t1>/<t2>')    # Arguments are integers: seconds since epoch
def history(node, t1, t2):
    weather_start_time=datetime.datetime.fromtimestamp(int(t1)).strftime('%Y-%m-%d %H:%M:%S')
    weather_end_time=datetime.datetime.fromtimestamp(int(t2)).strftime('%Y-%m-%d %H:%M:%S')

    node_history_data=extract_history_weather_data(node, weather_start_time, weather_end_time)
    return node_history_data
    # TODO: Insert rest of code here from lambda_history
    #return '{"key":"value", "hey": "it also works"}'

if __name__ == '__main__':
    app.run()
