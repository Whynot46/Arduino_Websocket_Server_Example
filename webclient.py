#uvicorn webclient:application --reload --port 80
import websockets
import asyncio
import json
import logging
import sys
from datetime import datetime
from typing import Iterator
from fastapi import FastAPI
from fastapi.requests import Request
from fastapi.responses import HTMLResponse, StreamingResponse
from fastapi.templating import Jinja2Templates
from starlette.responses import Response
from time import time
import uvicorn
from write_to_scv import write_to_scv


recording_period = 0.1 # период записи данных в .csv


logging.basicConfig(stream=sys.stdout, level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger(__name__)

application = FastAPI()
templates = Jinja2Templates(directory="templates")


@application.get("/", response_class=HTMLResponse)
async def index(request: Request) -> Response:
    return templates.TemplateResponse("index.html", {"request": request})


async def generate_random_data(request: Request) -> Iterator[str]:
    client_ip = request.client.host
    logger.info("Client %s connected", client_ip)

    last_value = ''
    fixed_time = time()

    async with websockets.connect("ws://192.168.1.10", ping_interval=None) as websocket:

        while True:
            date = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            value = await websocket.recv()

            if not value:
                value = last_value
            else: last_value = value

            shock_sensor = value[0]
            voice_sensor = value[1:]

            json_data = json.dumps(
                {
                    "time": date,
                    "shock_sensor": shock_sensor,
                    "voice_sensor": voice_sensor,
                }
            )
            yield f"data:{json_data}\n\n"

            current_time = time()
            if current_time - fixed_time > recording_period:
                #write_to_scv(date, voice_sensor)
                fixed_time = time()

            await asyncio.sleep(0.0005)


@application.get("/chart-data")
async def chart_data(request: Request) -> StreamingResponse:
    response = StreamingResponse(generate_random_data(request), media_type="text/event-stream")
    response.headers["Cache-Control"] = "no-cache"
    response.headers["X-Accel-Buffering"] = "no"
    return response

if __name__=="__main__":
    uvicorn.run("webclient:application", port=8080)