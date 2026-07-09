FROM python:3.12-slim

WORKDIR /app

RUN pip install --no-cache-dir httpx

COPY server.py .

EXPOSE 8765

CMD ["python3", "-u", "server.py"]
