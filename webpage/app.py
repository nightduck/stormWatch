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

@app.route('/update/<t1>/<t2>')     # Arguments are integers: seconds since epoch
def update(t1, t2):
    lightning_start_time=datetime.datetime.fromtimestamp(int(t1)).strftime('%Y-%m-%d %H:%M:%S')
    lightning_end_time=datetime.datetime.fromtimestamp(int(t2)).strftime('%Y-%m-%d %H:%M:%S')
    weather_start_time=datetime.datetime.fromtimestamp(int(t2)).strftime('%Y-%m-%d %H:%M:%S')

    # Insert rest of code here from lambda_test
    return '{"key":"value", "hey": "it works"}'

@app.route('/history/<t1>/<t2>')    # Arguments are integers: seconds since epoch
def history(t1, t2):
    weather_start_time=datetime.datetime.fromtimestamp(int(t1)).strftime('%Y-%m-%d %H:%M:%S')
    weather_end_time=datetime.datetime.fromtimestamp(int(t2)).strftime('%Y-%m-%d %H:%M:%S')

    # TODO: Insert rest of code here from lambda_history
    return '{"key":"value", "hey": "it also works"}'

if __name__ == '__main__':
    app.run()
