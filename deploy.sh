echo "Reverting config file changes..."
git restore **/*.yaml

echo "Pulling from remote..."
git pull

echo "Building new deployment..."
npm --prefix ./web/LED-Wall-Website run build

echo "Updating distribution..."
rm -rf ../dist/*
cp -r ./web/LED-Wall-Website/dist/* ../dist

echo "Starting virtual environment..."
source ./venv/bin/activate

echo "Restarting server if already running or starting server otherwise..."
cd web/web-server && pkill -HUP gunicorn || gunicorn --bind 127.0.0.1:5000 --daemon 'main:app'