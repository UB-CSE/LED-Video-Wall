echo "Pulling from remote..."
git pull

echo "Building new deployment..."
npm --prefix ./web/LED-Wall-Website run build

echo "Copying to distribution..."
cp ./web/LED-Wall-Website/dist ../dist