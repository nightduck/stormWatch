from flask import Flask

app = Flask(__name__)


@app.route('/')
def index():
    return app.send_static_file("index.html")

@app.route('/<path:path>')
def rsc_file(path):
    return app.send_static_file(path)

if __name__ == '__main__':
    app.run()
