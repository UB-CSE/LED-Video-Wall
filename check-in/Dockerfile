FROM python:3.14-alpine

COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .

CMD ["gunicorn", "-w", "4", "-b", "0.0.0.0:8000", "check_in_server:app"]